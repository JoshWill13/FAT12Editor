/*
Josh Williams
CSC 360, Assignment 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

void ChangeDirectory();

//Inserts specified size of file into an open data cluster
void InsertToDataCluster(FILE **f, FILE **FI, int cluster, int sizeOfData, int insertionPos){
	//printf("InsertToDataCluster\n");
	int pos = (31*512) + (cluster*512);
	char buf[512] = "";
	fseek(*FI, insertionPos, SEEK_SET);
	fread(buf, sizeof(char), sizeOfData, *FI);
	fseek(*f, pos, SEEK_SET);
	fwrite(buf, sizeof(char), sizeOfData, *f);
}

//Finds and empty Fat sector and temporarily fills it with 0xfff
int FindEmptyFat(FILE **f){ 
	//printf("FindEmptyFat\n");
	int FatLocation = 2;
	int pos = 515;
	int j;
	int BigByte;
	int LittleByte;
	int FatSector;
	char temp[4];
	for(j=1; j<=(2848/2); j++){ //j = 1 to skip first two sectors, last 1.5 byte as it does not count.
		FatSector = 0x00;
		fseek(*f, pos, SEEK_SET);
		fread(temp, sizeof(unsigned char), 3, *f);
		BigByte = temp[1] & 0x0f;
		LittleByte = temp[0] & 0xff;
		FatSector = (BigByte << 8) + LittleByte; //shift 8 bits to the left
		if(FatSector == 0x000){
			temp[1] = temp[1]+ 0x0f;
			temp[0] = 0xff;
			fseek(*f, pos, SEEK_SET);
			fwrite(&temp[0], sizeof(unsigned char), 1, *f);
			fseek(*f, pos+1, SEEK_SET);
			fwrite(&temp[1], sizeof(unsigned char), 1, *f);
			return FatLocation;
		}
		FatLocation++;
		FatSector = 0x000;
		BigByte = temp[2] & 0xff;
		LittleByte = temp[1] & 0xf0;
		FatSector = (BigByte << 4) + (LittleByte >> 4); //shift big byte left 4, small byte right 4
		if((FatSector == 0x000) && (j != 1424)){
			temp[2] = 0xff;
			temp[1] = temp[1] + 0xf0;
			fseek(*f, pos+1, SEEK_SET);
			fwrite(&temp[1], sizeof(unsigned char), 1, *f);
			fseek(*f, pos+2, SEEK_SET);
			fwrite(&temp[2], sizeof(unsigned char), 1, *f);
			return FatLocation;
		}
		FatLocation++;
		pos = pos+3;
		//Split 3 bytes into two 12 bit sections
	}
	return 0;
}

//changes previos Fat sector from 0xfff to point to next fat sector
void MarkFat(FILE **f, int FatNum, int NextFat){
	//printf("MarkFat\n");
	int pos = 512;
	int BigByte;
	int LittleByte;
	char temp[2];
	if(FatNum%2 == 0){
		FatNum = (FatNum*3)/2;
		pos = pos + FatNum;
		LittleByte = NextFat & 0xff;
		BigByte = (NextFat & 0xf00) >> 8;
		BigByte = BigByte + 0xf0;
		fseek(*f, pos, SEEK_SET);
		fread(temp, sizeof(unsigned char), 2, *f);
		temp[0] = (temp[0] & LittleByte);
		temp[1] = (temp[1] & BigByte);
		fseek(*f, pos, SEEK_SET);
		fwrite(&temp[0], sizeof(unsigned char), 1, *f);
		fseek(*f, pos+1, SEEK_SET);
		fwrite(&temp[1], sizeof(unsigned char), 1, *f);
	}else{
		FatNum = ((FatNum-1)*3)/2;
		pos = pos + FatNum + 1;
		LittleByte = (NextFat & 0x0f) << 4;
		LittleByte = LittleByte + 0x0f;
		BigByte = (NextFat & 0xff0) >> 4;
		fseek(*f, pos, SEEK_SET);
		fread(temp, sizeof(unsigned char), 2, *f);
		temp[0] = (temp[0] & LittleByte);
		temp[1] = (temp[1] & BigByte);
		fseek(*f, pos, SEEK_SET);
		fwrite(&temp[0], sizeof(unsigned char), 1, *f);
		fseek(*f, pos+1, SEEK_SET);
		fwrite(&temp[1], sizeof(unsigned char), 1, *f);
	}
}

//Inserts file into correct directory and delegates taks
int InsertFile(FILE **f, FILE **FI, int pos, int size, int NumSectors, char *filename){
	//printf("InsertFile\n");
	if(pos == 0){
		pos = 512*19;
	}
	char Name[9] = "";
	fseek(*f, pos, SEEK_SET);
	fread(Name, sizeof(unsigned char), 8, *f);
	while(Name[0] != 0x00){
		pos = pos+32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}

	int InitialSize = size;
	int tmp = 0;
	int insertionPos = 0;
	int FirstCluster;
	while(size > 0){
		int tmp2 = FindEmptyFat(&*f); //Mark with last cluster oxfff
		if(tmp2 == 0){
			printf("Can't find empty FAT position.\n");
			exit(0);
		}
		if(tmp!= 0) {
			MarkFat(&*f, tmp , tmp2);
		}else{
			FirstCluster = tmp2;
		}
		tmp = tmp2;

		if(size >= 512){
			InsertToDataCluster(&*f, &*FI, tmp, 512, insertionPos);
		}else{
			InsertToDataCluster(&*f, &*FI, tmp, size, insertionPos);
		}
		size = size - 512;
		insertionPos = insertionPos + 512;
	}

	ChangeDirectory(filename, &*f, pos, InitialSize, NumSectors, FirstCluster);
	return 0;
}


//Returns cluster number of a directory, otherwise returns 0
int FindDirectory(FILE **f, char *dir, int sector){
	//printf("FindDirectory\n");
	char Name[9] = "";
	char FirstLogicalCluster[3]= "";
	char AttributeField[2] = "";
	int pos;
	if(sector == 0){
		pos = 512*19;
	}else{
		pos = 31*512;
		pos = pos + (sector*512);
	}
	
	fseek(*f, pos, SEEK_SET);
	fread(Name, sizeof(unsigned char), 8, *f);
	while(Name[0] != 0x00){
		fseek(*f, (pos +26), SEEK_SET);
		fread(FirstLogicalCluster, sizeof(unsigned char), 2, *f);
		if(((FirstLogicalCluster[1] == 0x00) && (FirstLogicalCluster[0] == 0x00)) || ((FirstLogicalCluster[1] == 0x01) && (FirstLogicalCluster[0] == 0x00))){
			//Ignore this file
			pos = pos + 32;
			fseek(*f, pos, SEEK_SET);
			fread(Name, sizeof(unsigned char), 8, *f);
			continue;
		}
		fseek(*f, (pos +11), SEEK_SET);
		fread(AttributeField, sizeof(unsigned char), 1, *f);
			if(AttributeField[0] == 0x0f || Name[0]== 0xef || Name[0]== 0x2e){
			//Ignore this file
			pos = pos + 32;
			fseek(*f, pos, SEEK_SET);
			fread(Name, sizeof(unsigned char), 8, *f);
			continue;
		}

		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			if(strlen(dir) < 7){
				if(strncmp(Name, dir, strlen(dir))==0){
					return FLC;
				}else{
					return FindDirectory(&*f, dir, FLC);
				}
			}else{
				if(strncmp(Name, dir, 7)==0){
					return FLC;
				}else{
					return FindDirectory(&*f, dir, FLC);
				}
			}
		}
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
	return 0;
}

int TotalDiskSize(FILE **f){
	//printf("TotalDiskSize\n");
	int bytesPerSector;
	int sectorsInFilesystem;
	char num[3];
	int pos =11;
	fseek(*f, pos, SEEK_SET);
	fread(num, sizeof(unsigned char), 2, *f);
	bytesPerSector = num[0] | num[1] << 8;
	pos =19;
	fseek(*f, pos, SEEK_SET);
	fread(num, sizeof(unsigned char), 2, *f);
	sectorsInFilesystem = num[0] | num[1] << 8;

	return (bytesPerSector*sectorsInFilesystem);
}

int FreeFatSectors(FILE **f){
	//printf("FreeFatSectors\n");
	int pos = 515;
	char temp[4];
	int j;
	int freeFATSectors = 0;
	int BigByte;
	int LittleByte;
	int FatSector;
	for(j=1; j<=(2848/2); j++){ //j = 1 to skip first two sectors, last 1.5 byte as it does not count.
		FatSector = 0x00;
		fseek(*f, pos, SEEK_SET);
		fread(temp, sizeof(unsigned char), 3, *f);
		pos = pos+3;
		BigByte = temp[1] & 0x0f;
		LittleByte = temp[0] & 0xff;
		FatSector = (BigByte << 8) + LittleByte; //shift 8 bits to the left
		if(FatSector == 0x00){
			freeFATSectors++;
		}
		FatSector = 0x00;
		BigByte = temp[2] & 0xff;
		LittleByte = temp[1] & 0xf0;
		FatSector = (BigByte << 4) + (LittleByte >> 4); //shift big byte left 4, small byte right 4
		if((FatSector == 0x00) && (j != 1424)){
			freeFATSectors++;
		}
	}
	return freeFATSectors;
}

//Inserts directory info about the file we're inserting
void ChangeDirectory(char *file, FILE **f, int pos, int size, int sectorsNeeded, int FLC){
	//printf("ChangeDirectory\n");
	char DirInsert[32];
	memset(DirInsert, 0, 32);
	struct stat attr;
	stat(file, &attr);
	char year[6];char month[6];char day[6]; char hour[6]; char minute[6]; char second[6];
	strftime(year, 5, "%Y", localtime(&attr.st_mtime));
	strftime(month, 5, "%m", localtime(&attr.st_mtime));
	strftime(day, 5, "%d", localtime(&attr.st_mtime));
	strftime(hour, 5, "%H", localtime(&attr.st_mtime));
	strftime(minute, 5, "%M", localtime(&attr.st_mtime));
	strftime(second, 5, "%S", localtime(&attr.st_mtime));
	int y = atoi(year); int m = atoi(month); int d = atoi(day);
	int h = atoi(hour); int min = atoi(minute); int s = atoi(second);
	int i;
	
	for(i = 0; i< strlen(file); i++){
		file[i]  = toupper(file[i]);
	}
	char *PreExtension = strtok(file, ".");
	char *Extension =  strtok(NULL, ".");
	
	i =0;
	while(i<8 && i<strlen(PreExtension)){
		DirInsert[i] = PreExtension[i];
		i++;
	}
	
	i = 0;
	while(i<3 && i<strlen(Extension)){
		DirInsert[8+i] = Extension[i];
		i++;
	}
	
	//26-27 FirstCluster
	char tmp[4];
	tmp[0] = FLC & 0xff;
	tmp[1] = (FLC & 0xff00) >> 8;
	DirInsert[26] = tmp[0];
	DirInsert[27] = tmp[1];

	//28-31 size of file
	tmp[0] = size & 0xff;
	tmp[1] = (size & 0xff00) >> 8;
	tmp[2] = (size & 0xff0000) >> 16;
	tmp[3] = (size & 0xff000000) >> 24;
	DirInsert[28] = tmp[0];
	DirInsert[29] = tmp[1];
	DirInsert[30] = tmp[2];
	DirInsert[31] = tmp[3];
	
	//14-17 Creation time - creation date
	y = y -1980;
	DirInsert[14] = (0b00000111 & min) << 5;
	DirInsert[14] = DirInsert[14] + (0b00011111 & s);
	DirInsert[15] = (0b00011111 & h) << 3;
	DirInsert[15] = DirInsert[15] + ((0b00111000& min)>>3);
	DirInsert[16] = (0b00000111 & m) <<5;
	DirInsert[16] = DirInsert[16] +(0b00011111 & d);
	DirInsert[17] = (0b01111111 & y) << 1;
	DirInsert[17] = DirInsert[17] + ((0b00001000 & m)>> 3);
	
	//22-25 Last Write time - Last Write date
	DirInsert[22] = (0b00000111 & min) << 5;
	DirInsert[22] = DirInsert[22] + (0b00011111 & s);
	DirInsert[23] = (0b00011111 & h) << 3;
	DirInsert[23] = DirInsert[23] + ((0b00111000& min)>>3);
	DirInsert[24] = (0b00000111 & m) <<5;
	DirInsert[24] = DirInsert[24] +(0b00011111 & d);
	DirInsert[25] = (0b01111111 & y) << 1;
	DirInsert[25] = DirInsert[25] + ((0b00001000 & m)>> 3);
	
	fseek(*f, pos, SEEK_SET);
	fwrite(DirInsert, sizeof(unsigned char), 32, *f);
}

//gets directory and file info from input, also opens files
int main(int argc, char* argv[]){
	
	if(argc < 3){
		printf("Error: Second or Third argument not supplied\n");
		exit(1);
	}
	
	FILE *f = fopen(argv[1], "rb+");
	FILE *FI;
	if(f == NULL){
		printf("Incorrect IMA file supplied\n");
		exit(0);
	}
	
	char *input = argv[2];
	int j = 0;
	int DirCount = 0;
	while(input[j] != '\0'){
		if(input[j]== '/'){
			DirCount++;
		}
		j++;
	}
	
	if(input[0] == '/'){
		char *tok;
		char *file;
		char *dir;
		tok = strtok(input, "/");
		int i = 1;
		while(tok != NULL){
			i++;
			dir = file;
			file =  tok;
			tok = strtok(NULL, "/");
		}
		
		FI = fopen(file, "rb+");
		if(FI == NULL){
			printf("%s doesn't exist\n", file);
			exit(0);
		}
		fseek(FI, 0L, SEEK_END);
		int size = ftell(FI);
		int FFS = FreeFatSectors(&f);
		int SectorsNeeded = size/512;
		if((size%512) != 0){
			SectorsNeeded++;
		}
		if((FFS - SectorsNeeded) < 0){
			printf("Error: Not enough free space.\n");
			exit(0);
		}
		fseek(FI, 0L, SEEK_SET);
		for(i = 0; i< strlen(dir); i++){
			dir[i]  = toupper(dir[i]);
		}
		int dirPos = FindDirectory(&f, dir, 0);
		if(dirPos == 0){
			printf("No directory match was found for %s.\n", dir);
			exit(0);
		}
		dirPos = (31*512) + (dirPos*512);

		InsertFile(&f, &FI, dirPos, size, SectorsNeeded, file);
	}else{
		
		FI = fopen(input, "rb+");
		if(f == NULL){
			printf("%s doesn't exist (case sensitive).\n", input);
			exit(0);
		}
		fseek(FI, 0L, SEEK_END);
		int size = ftell(FI);
		int FFS = FreeFatSectors(&f);
		int SectorsNeeded = size/512;
		if((size%512) != 0){
			SectorsNeeded++;
		}
		
		if((FFS - SectorsNeeded) < 0){
			printf("Error: Not enough free space.\n");
			exit(0);
		}
		fseek(FI, 0L, SEEK_SET);
		InsertFile(&f, &FI, 0, size, SectorsNeeded, input);
	}
	
	fclose(FI);
	fclose(f);
	return 0;
}