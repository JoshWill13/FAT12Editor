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

//GLOBAL VARIABLES//
int NumOfFiles = 0;

void GoToDataSector();


int GetFreeFatSectors(FILE **f){
	int pos = 515;
	char temp[4];
	int j;
	int freeFATSectors = 0;
	int BigByte;
	int LittleByte;
	int FatSector;
	for(j=1; j<=(2848/2); j++){ //j = 1 to skip first two sectors, last 1.5 byte as it does not count.
								//Split 3 bytes into two 12 bit sections
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


int GetTotalDiskSize(FILE **f){
	int bytesPerSector;
	int sectorsInFilesystem;
	int pos =11;
	char num[3];
	
	fseek(*f, pos, SEEK_SET);
	fread(num, sizeof(unsigned char), 2, *f);
	bytesPerSector = num[0] | num[1] << 8;
	
	pos =19;
	fseek(*f, pos, SEEK_SET);
	fread(num, sizeof(unsigned char), 2, *f);
	sectorsInFilesystem = num[0] | num[1] << 8;
	
	return (bytesPerSector*sectorsInFilesystem);
}


char * GetVolumeLabel(FILE **f, char* volumeLabel){
	int pos = 43;	//43 the location of volume label
	fseek(*f, pos, SEEK_SET);
	fread(volumeLabel,sizeof(unsigned char), 11, *f);

	if(volumeLabel[0] == ' '){
		char DiskName[13] = "";
		
		pos = 512 * 19;	
		fseek(*f, pos, SEEK_SET);//read in name, extension, and attributes
		fread(DiskName, sizeof(unsigned char), 12, *f); 

		while(DiskName[0] != 0x00){
			if(DiskName[11] == 8){ //Checking if VolumeLabel is marked
				int i;
				for(i = 0; i<11; i++){
					volumeLabel[i] = DiskName[i];
				}
			}
			pos = pos +32; //move to the beginning of the next directory entry
			fseek(*f, pos, SEEK_SET);
			fread(DiskName, sizeof(unsigned char), 12, *f);
		}
	}
	char *VL = volumeLabel;
	return VL;
}


void CalculateNumFiles(FILE **f){
	//Get number of files in root and subdirectories
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
			//Ignore this file / skip iteration of while loop
			pos = pos + 32;
			fseek(*f, pos, SEEK_SET);
			fread(Name, sizeof(unsigned char), 8, *f);
			continue;
		}
		
		fseek(*f, (pos +11), SEEK_SET);
		fread(AttributeField, sizeof(unsigned char), 1, *f);
		if((AttributeField[0] & 0b00011001) == 0){
			NumOfFiles++;
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			GoToDataSector(FLC, &*f);
		}
		
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
}


void GoToDataSector(int sector, FILE** f){
	
	int pos = (31+sector)*512; //We start at 31 as data sectors 0,1 dont exist.
	char FirstLogicalCluster[3]= "";
	char AttributeField[2] = "";
	char Name[9] = "";
	
	fseek(*f, pos, SEEK_SET);
	fread(Name, sizeof(unsigned char), 8, *f);
	
	while(Name[0] != 0x00){
		fseek(*f, (pos +26), SEEK_SET);
		fread(FirstLogicalCluster, sizeof(unsigned char), 2, *f);
		
		if(((FirstLogicalCluster[1] == 0x00) && (FirstLogicalCluster[0] == 0x00)) || ((FirstLogicalCluster[1] == 0x01) && (FirstLogicalCluster[0] == 0x00))){
			//Ignore this file / skip iteration of while loop
			pos = pos + 32;
			fseek(*f, pos, SEEK_SET);
			fread(Name, sizeof(unsigned char), 8, *f);
			continue;
		}
		
		fseek(*f, (pos +11), SEEK_SET);
		fread(AttributeField, sizeof(unsigned char), 1, *f);
		
		if(AttributeField[0] == 0x0f || Name[0]== 0xef || Name[0]== 0x2e){
			//Ignore this file / skip iteration of while loop
			pos = pos + 32;
			fseek(*f, pos, SEEK_SET);
			fread(Name, sizeof(unsigned char), 8, *f);
			continue;
		}
		
		if((AttributeField[0] & 0b00001000) == 0 && (AttributeField[0] & 0b00010000) == 0 ){
			NumOfFiles++;
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			GoToDataSector(FLC, &*f);
		}
		
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
}


int main(int argc, char* argv[]){
	
	if(argc < 2){
		printf("Error: Second argument not supplied\n");
		exit(1);
	}
	
	FILE *f = fopen(argv[1], "rb+");
	int pos;
	char OSName[9] = "";
	
	//Get OSNAME
	pos = 3;
	fseek(f, pos, SEEK_SET);
	fread(OSName, sizeof(unsigned char), 8, f);

	//GetVolumeLabel
	char *volumeLabel = malloc(sizeof(char));
	GetVolumeLabel(&f, volumeLabel);
	
	//Get TotalDiskSize, Num of FreeFatSectors, Num of Files
	int TotalDiskSize = GetTotalDiskSize(&f);
	int freeFATSectors = GetFreeFatSectors(&f);
	CalculateNumFiles(&f);
	
	// Getting Number of Fat Copies, & SectorsPerFat
	char TempFat[3] = "";
	int NumFatCopies;
	int SectorsPerFat;
	
	fseek(f, 16, SEEK_SET);
	fread(TempFat, sizeof(unsigned char), 1, f);
	NumFatCopies = TempFat[0];

	fseek(f, 22, SEEK_SET);
	fread(TempFat, sizeof(unsigned char), 2, f);
	SectorsPerFat = TempFat[0] + (TempFat[1] << 8);
	
	
	//----------------------PRINT SECTION----------------------------//
	printf("OS Name: %s\n", OSName);
	printf("Label of the disk: %s\n", volumeLabel);
	printf("Total size of the disk: %d\n", TotalDiskSize);
	printf("Free size of the disk:  %d\n", freeFATSectors*512);
	printf("\n");
	printf("==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n", NumOfFiles);
	printf("\n");
	printf("==============\n");
	printf("Number of FAT copies: %d\n", NumFatCopies);
	printf("Sectors per FAT: %d\n", SectorsPerFat);
	
	
	fclose(f);
	free(volumeLabel);
	return 0;
}