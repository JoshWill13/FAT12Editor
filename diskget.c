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

//GLOBAL VARIABLE//
int CopySucceeded = 0;

void FindFilesRecursive();
void WriteToFile();

void FindFiles(FILE **f, char file[20]){
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
		if((AttributeField[0] & 0b00011001) == 0){
			
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
			int i;
			int NameEqual = 0;
			if(strlen(file) < 7){
				for(i =0; i < strlen(file); i++){ //Deal with some NameStorageIssues
					/*if(Name[i] == 0x7e){
						break;
					}*/
					if(Name[i] != file[i]){
						NameEqual = 1;
						break;
					}
				}
			}else{
				for(i =0; i < 7; i++){ //Deal with some NameStorageIssues
					if(Name[i] == 0x7e){
						break;
					}
					if(Name[i] != file[i]){
						NameEqual = 1;
						break;
					}
				}
			}
			if(NameEqual==0){
				CopySucceeded = 1;
				//printf("Found the file: %s\n", file);
				int FLC =(FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
				WriteToFile(&*f, file, pos, fileSize, FLC);
			}
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);

			FindFilesRecursive(FLC, &*f, file);
		}
		
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
}

void FindFilesRecursive(int sector, FILE **f, char file[20]){
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
		if((AttributeField[0] & 0b00001000) == 0 && (AttributeField[0] & 0b00010000) == 0 ){
			char size[5];
			fseek(*f, (pos + 28), SEEK_SET);
			fread(size, sizeof(unsigned char), 4, *f);
			int fileSize = (size[0] & 0xff) + ((size[1] & 0xff) << 8) + ((size[2] & 0xff) << 16) + ((size[3] & 0xff) << 24);
			int i;
			int NameEqual = 0;
			if(strlen(file) < 7){
				for(i =0; i < strlen(file); i++){ //Deal with some NameStorageIssues
					/*if(Name[i] == 0x7e){
						break;
					}*/
					if(Name[i] != file[i]){
						NameEqual = 1;
						break;
					}
				}
			}else{
				for(i =0; i < 7; i++){
					if(Name[i] == 0x7e){ //Deal with some NameStorageIssues
						break;
					}
					if(Name[i] != file[i]){
						NameEqual = 1;
						break;
					}
				}
			}
			if(NameEqual==0){
				CopySucceeded = 1;
				//printf("Found the file: %s\n", file);
				int FLC =(FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
				WriteToFile(&*f, file, pos, fileSize, FLC);
			}
		}
		
		if((AttributeField[0] & 0b00010000)== 0b00010000){
			int FLC = (FirstLogicalCluster[1] << 8) + FirstLogicalCluster[0];
			if(FLC != sector){
				char size[5];
				fseek(*f, (pos + 28), SEEK_SET);
				fread(size, sizeof(unsigned char), 4, *f);

				FindFilesRecursive(FLC, &*f, file); 
			}
		}
		pos = pos + 32;
		fseek(*f, pos, SEEK_SET);
		fread(Name, sizeof(unsigned char), 8, *f);
	}
}


void WriteToFile(FILE **f, char file[20], int InitalPosition, int size, int flc){
	FILE *cf = fopen(file, "wb");
	if(cf == NULL){
		printf("Can't open file!\n");
		exit(0);
	}
	//printf("Writing to %s.\n", file);
	char buf[513] = "";
	int pos = (31+flc)*512;
	int writePosition = 0;
	fseek(*f, pos, SEEK_SET);
	while(size > 512){
		fread(buf, sizeof(char), 512, *f);
		fseek(cf, writePosition, SEEK_SET);
		fwrite(buf, sizeof(char), 512, cf);
		size = size - 512;
		int ReadFat;
		int FatSector = 0x00;
		int BigByte;
		int LittleByte;
		char temp [3];
		if(flc % 2 == 0){
			ReadFat = 512 + (flc*3/2);
			fseek(*f, ReadFat, SEEK_SET);
			fread(temp, sizeof(unsigned char), 2, *f);
			BigByte = temp[1] & 0x0f;
			LittleByte = temp[0] & 0xff;
			FatSector = (BigByte << 8) + LittleByte; //shift 8 bits to the left
			pos = (31+FatSector)*512;
			fseek(*f, pos, SEEK_SET);
			flc = FatSector;
		}else{
			ReadFat = 512 + ((flc-1)*3/2)+1;
			fseek(*f, ReadFat, SEEK_SET);
			fread(temp, sizeof(unsigned char), 2, *f);
			BigByte = temp[1] & 0xff;
			LittleByte = temp[0] & 0xf0;
			FatSector = (BigByte << 4) + (LittleByte >> 4); //shift big byte left 4, small byte right 4
			pos = (31+FatSector)*512;
			fseek(*f, pos, SEEK_SET);
			flc = FatSector;
		}
		writePosition = writePosition + 512;
	}

	memset(buf, 0, 513); // Clear buffer as we are inserting a size smaller than 512 bytes.
	fread(buf, sizeof(char), size, *f);
	fseek(cf, writePosition, SEEK_SET);
	fwrite(buf, sizeof(char), size, cf);
	fclose(cf);
}



int main(int argc, char* argv[]){
	if(argc < 3){
		printf("Error: Second or Third argument not supplied\n");
		exit(1);
	}
	
	FILE *f = fopen(argv[1], "rb+");
	char* input = argv[2];
	char file[20] = "";
	int i;
	
	for(i = 0; i< strlen(input); i++){
		file[i]  = toupper(input[i]);
	}
	
	printf("Searching for %s\n", file);
	FindFiles(&f, file);
	
	if(CopySucceeded == 0){
		printf("File Not Found.\n");
		exit(0);
	}else{
		printf("%s has been found.\n", file);
	}
	
	fclose(f);
	return 0;
}