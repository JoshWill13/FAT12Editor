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
int NumOfFiles;

void PrintFilesRecursive();

void PrintDateTime(int pos, FILE **f){
	char DateTime[5];
	fseek(*f, (pos +14), SEEK_SET);
	fread(DateTime, sizeof(unsigned char), 4, *f);
	int day = (DateTime[2] & 0b00011111);
	int month = ((DateTime[2] & 0b11100000) >> 5) + ((DateTime[3] & 0b00000001) << 3);
	int year = ((DateTime[3] & 0b11111110) >> 1 )+ 1980;
	int hour = ((DateTime[1]& 0b11111000) >> 3); 
	int minute = ((DateTime[0] & 0b11100000) >> 5) + ((DateTime[1] & 0b00000111) << 3);
	//int seconds = (DateTime[0] & 0b00011111); 	//Only if we wanted seconds
	//printf("%d-%d-%d  %d:%d:%d\n", year, month, day, hour, minute, seconds);
	printf("%d-%d-%d  %d:%d\n", year, month, day, hour, minute);
}

//Go through root directory
//When goDir == 1, we got into directories
void PrintFiles(FILE **f, int goDir){
	char Name[9] = "";
	char FirstLogicalCluster[3]= "";
	char AttributeField[2] = "";
	int pos = 512*19;
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
		if(((AttributeField[0] & 0b00011001) == 0)&& goDir ==0){
			NumOfFiles++;
			
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
			printf("F %*d %*s ", 10, fileSize, 20, Name);
			
			PrintDateTime(pos, &*f);
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
			if(goDir == 0){
				printf("D %*d %*s ", 10, fileSize, 20, Name);
				PrintDateTime(((31+FLC)*512), &*f);
			}else{
				printf("\n%s\n", Name);
				printf("==================\n");
				PrintFilesRecursive(FLC, &*f, 0);
			}
		}
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
	//Once we have found all files in a directory we enter other directories
	if(goDir == 0){
		PrintFiles(&*f, 1);
	}
}

//Go through subdirectories
//When goDir == 1, we got into directories
void PrintFilesRecursive(int sector, FILE **f, int goDir){
	int pos = (31+sector)*512; //We start at 31 as data sectors 0,1 dont exist.
	char FirstLogicalCluster[3]= "";
	char AttributeField[2] = "";
	char Name[9] = "";
	
	fseek(*f, pos, SEEK_SET);
	fread(Name, sizeof(unsigned char), 8, *f);
	
	while(Name[0] != 0x00){
		fseek(*f, (pos +26), SEEK_SET);
		fread(FirstLogicalCluster, sizeof(unsigned char), 2, *f);
		if(((FirstLogicalCluster[1] == 0x00) && (FirstLogicalCluster[0] == 0x00)) || ((FirstLogicalCluster[1] == 0x00) && (FirstLogicalCluster[0] == 0x01))){
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
		if((AttributeField[0] & 0b00001000) == 0 && (AttributeField[0] & 0b00010000) == 0 && (goDir == 0)){
			NumOfFiles++;
			
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
			printf("F %*d %*s ", 10, fileSize, 20, Name);
			
			PrintDateTime(pos, &*f);
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			if(FLC != sector){
				char size[5];
				fseek(*f, (pos + 28), SEEK_SET);
				fread(size, sizeof(unsigned char), 4, *f);
				int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
				if(goDir == 0){
					printf("D %*d %*s ", 10, fileSize, 20, Name);
					PrintDateTime(((31+FLC)*512), &*f);
				}else{
					printf("\n%s\n", Name);
					printf("==================\n");
					PrintFilesRecursive(FLC, &*f, 0); //**************************************CHANGE
				}
			}
		}
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
	//Once we have found all files in a directory we enter other directories
	if(goDir == 0){
		PrintFilesRecursive(sector, &*f, 1);
	}
}

int main(int argc, char* argv[]){

	if(argc < 2){
		printf("Error: Second argument not supplied\n");
		exit(1);
	}
	
	FILE *f = fopen(argv[1], "rb+");
	int pos;
	
	pos = 43;	//43 the location of volume label
	char  volumeLabel[12] = "";
	fseek(f, pos, SEEK_SET);
	fread(volumeLabel,sizeof(unsigned char), 11, f);
	
	if(volumeLabel[0] == ' '){
		pos = 512 * 19;	
		char DiskName[13] = "";
		fseek(f, pos, SEEK_SET);
		fread(DiskName, sizeof(unsigned char), 11, f);
		while(DiskName[0] != 0x00){
			if(DiskName[11] == 8){ // Check attribues if set to volumeLabel
				int i;
				for(i = 0; i<11; i++){
					volumeLabel[i] = DiskName[i];
				}
			}
			pos = pos +32; //move to the beginning of the next directory entry
			fseek(f, pos, SEEK_SET);
			fread(DiskName, sizeof(unsigned char), 12, f);
		}
	}
	
	
	printf("%s\n", volumeLabel);
	printf("==================\n");
	PrintFiles(&f, 0);
	fclose(f);
	return 0;
}

