#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h> 
/*
#define FILEPATH "/home/cristiam/minix/pr/usr/lab6/"
#define MINIXPATH "/home/cristiam/minix/pr/usr/"*/
#define FILEPATH "/usr/usr/lab6/"
#define MINIXPATH "/usr/usr/"
#define BLOCKSIZE 8 /*bytes*/

struct vd{
	int size;/*In bytes*/
	int files;
	int firstaddr;
	int blocks;
	int freeblocks;	
	int blockid;
	int available;
	struct vd * realnxt;
	struct vd * filenext;
};
struct vd *Sblock=NULL;
struct vd *current=NULL;
struct vd *end=NULL;
struct filesList{
	char name[15];
	int size;
	int address;
}fl[500];

int randomSize(void){

	/*Size of the files are between 8 and 500 bytes*/
	int max = 500;
	int min = 10;
	int num = (rand() % (max - min + 1)) + min;
	return num;
}
void createFile(char * file, int sel){
	int fd;
	int result;
	char str[80];
	/*0 for VD else for MINIX*/
	if (sel==0)
		strcpy(str,FILEPATH);
	else
		strcpy(str,MINIXPATH);
	strcat(str,file);
	fd = open(str, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
	if (fd == -1) {
		perror("Error opening file for writing");
		/*return 1;*/
	}
	result = lseek(fd, randomSize()-1, SEEK_SET);
	if (result == -1) {
		close(fd);
		perror("Error calling lseek() to 'stretch' the file");
		/*return 1;*/
	}

	/* write just one byte at the end */
	result = write(fd, "", 1);
	if (result < 0) {
		close(fd);
		perror("Error writing a byte at the end of the file");
		/*return 1;*/
	}

	/* do other things here */

	close(fd);
}

int getSize(char * file,int sel){
	struct stat buffer;
	int         status;
	int size;
	char str[80];
	/*0 for VD else for MINIX*/
	if (sel==0)
		strcpy(str,FILEPATH);
	else
		strcpy(str,MINIXPATH);
	strcat(str,file);
	printf("Getting file's size: %s\n",str);
	status = stat(str, &buffer);
	if(status == 0) {
		printf("File size: %ld\n",buffer.st_size);
		size=buffer.st_size;
	}
	else{
		printf("File: %s, does not exist\n",file);	
	}
	return size;
}
int calculateBlocks(int size){
	int blocks;
	blocks=size/BLOCKSIZE;
	return blocks;
}

int getPosFileTable(void){
	int i=0;
	while (fl[i].size!='\0'){
		i++;/*Look for first free space in table*/
	}
	return i;
}

/*Once validated, assign the available blocks*/
int assignBlocks(int blocks){
	int i;
	int pos;/*First allocation space the file table available*/
	struct vd *temp = (struct vd*) malloc(sizeof(struct vd));
	Sblock->files+=1;
	current=Sblock->realnxt;
	temp=current;
	/*printf("Current block's ID: %d\n",current->blockid);*/
	/*Look for next free node*/
	for(i=0;i<blocks;i++){
		if(current != temp){
			current->filenext=temp;	
			current=temp;	
		}
		while(temp->available!=1){
			temp=temp->realnxt;/*Find closest available block*/			
		}
		/*Assign address for file 1*/
		if(i==0){/*Save address in file table*/
		current=temp;/*current goes to the first available block found by 'temp'*/
		pos=getPosFileTable();/*Look for first free space in table*/
		fl[pos].address=temp->blockid;
		}
		temp->available=0;
		Sblock->freeblocks-=1;
	}/*for*/
	current->filenext=temp;
	current=temp;
	current->filenext=NULL;/*End of file*/
	printf("Last block's ID: %d\tBlocks assigned!\n",current->blockid);
	return pos;
}
void exploreVDFiles(void){
	int i;
	int j=0;
	printf("\n\t\t------ FILE TABLE ------\n");
	for (i=0;j<Sblock->files;i++){
	/*for(i=0;i<20;i++){*/
		if(fl[i].size=='\0'){
		printf("fl[%d]: NULL\n",i+1);
		}
		else{
		printf("fl[%d]: %d B\taddress: %d\tfile: %s\n",i+1,fl[i].size,fl[i].address,fl[i].name);
		j++;
		}
	}	
}
/*Look for file, by name*/
/*returns the position in the File table */
int findAllocationbyName(char * file){
	int i;
	int j=0;
	int val=-2;
	for (i=0;j<Sblock->files;i++){
		if(fl[i].size=='\0'){
		/*printf("fl[%d]= NULL\n",i);*/
		}
		else{
		/*printf("fl[%d]: %d B\taddress: %d\tfile: %s\n",i+1,fl[i].size,fl[i].address,fl[i].name);*/
		/*printf("fl[%d]= %s, file= %s\n",i,fl[i].name,file);*/
				if (strcmp(fl[i].name, file)==0){
				printf("File: %s, is in the system\n",file);
				val=i;		
				}
		j++;
		}
	}	
	val = (val>=0)?val:-1;
	return val;
}
void copyfromMinix(char * file){

	struct stat buffer;
	int         status;
	int blocks;
	int size;
	int pos;
	int val;
	char str[80];
	strcpy(str,MINIXPATH);
	strcat(str,file);/*Concatenate path*/
	printf("Copying... %s\n",str);
	status = stat(str, &buffer);
	val=findAllocationbyName(file);
	/*printf("VALVALVALVAL %d\n",val);*/
	if(status == 0 && val == -1 ) {
		/*printf("File size: %ld\n",buffer.st_size);*/
		size=buffer.st_size;
		blocks=calculateBlocks(size);
		if (blocks<Sblock->freeblocks){
			printf("File: %s\tsize: %d\trequires: %d blocks\n",file,size,blocks);
			/*printf("STRUCTURE FILES BEFORE ASSIGN BLOCKS:\n");*/
			/*printf("%d\n",fl[Sblock->files-1].address);*/
			pos=assignBlocks(blocks);
			printf("Address in file table: %d\t",pos+1);
			printf("Address in disk: %d\n\n",fl[pos].address);/*Assign values in File Table*/
			strcpy(fl[pos].name,file);
			fl[pos].size=size;
		}
		else{
			perror("Not enough free space in disk\n");
		}
	}
	else{
		if (val >= 0){printf("File: %s, already exists\n",file);}
		else {printf("File: %s, does not exist\n",file);}
	}	
}

void createmanualFile(void){
	char str[15];
	int sel;/*0 for VD*/
	printf("Enter name of file: ");
	scanf("%s",str);
	printf("Create in VD (0): ");
	scanf("%d",&sel);
	printf("Input: %s\nSel: %d\n",str,sel);
	createFile(str,sel);
	getSize(str,sel);
}
void createList(void){
	int i;
	printf("Size assigned: %d Bytes\n", Sblock-> size);
	printf("Creating %d blocks\n", Sblock-> blocks);
	for (i=2;i<Sblock->blocks+2;i++){
		struct vd *new = (struct vd*) malloc(sizeof(struct vd));
		current->realnxt=new;
		current->available=1;
		current=new;
		end=new;
		current->blockid=i;
	}
	current=Sblock->realnxt;
	printf("First block's ID: %d\n",current->blockid);
	printf("Last block's ID: %d\n",end->blockid);
}
int availableblocks(int _size){
	int blocks=(_size-sizeof(Sblock))/BLOCKSIZE;
	return blocks;
}
void createVD(int _size){
	int blocks;
	struct vd *link = (struct vd*) malloc(sizeof(struct vd));
	link->size = _size*1024;
	link->files =0;
	link->firstaddr=2;
	link->blockid=1;
	blocks=availableblocks(_size*1024);
	link->blocks=blocks;
	link->freeblocks=blocks;
	link->available=0;

	Sblock = link;
	current = link;
	end = link;
	printf("\n\t\t------ PREPARING DISK ------\n");
	createList();
}

/*Characteristics of the Virtual Disk*/
void Sblockchars(void){
	printf("\n\t\t------ VIRTUAL DISK ------\n");
	printf("-Size: %d Bytes\n",Sblock->size);	
	printf("-Number of blocks: %d\n",Sblock->blocks);
	printf("-Available blocks: %d\n",Sblock->freeblocks);
	printf("-Available space: %d Bytes\n",Sblock->freeblocks*BLOCKSIZE);
	printf("-Files: %d\n\n",Sblock->files);
}
void mapVD(void){
	int i,j,val;
	printf("\n\t\t------ DISK MAPPER ------\n");
	printf("*S ");
	current=Sblock->realnxt;
	for(i=2;i<Sblock->blocks+1;i++){
		if(current->blockid%20==1){val=i/10;}
		else{val=current->blockid%20;}
		if(current->available==1){printf("-%d ",val);}
		else{printf("*%d ",current->blockid%20);}
		if (i%20==0){printf("\n");}	
		current=current->realnxt;
	}
	printf("\n");
	current = Sblock->realnxt;
}
/*Once validated, unassign the assigned blocks*/
void unassignBlocks(int address){
	current=Sblock->realnxt;
	printf("Current address: %d\n",address);
	/*Traverse until find address of file*/
	while (current->blockid!=address){
		/*printf("Current blockid: %d address:%d\n",current->blockid,address);*/
		current=current->realnxt;
	}
	do{
	current->available=1;
	Sblock->freeblocks+=1;
	printf("deleting block: %d\n",current->blockid);
	current=current->filenext;
	}while(current->filenext!=NULL);
	current->available=1;
	Sblock->freeblocks+=1;
	printf("deleting block: %d\n",current->blockid);
}
void deletefile(char * file){
	int temp,addr;
	char str[80];
	strcpy(str,MINIXPATH);
	strcat(str,file);/*Concatenate path*/
	
	printf("\nDeleting... %s\n",str);
	addr=findAllocationbyName(file);
	printf("Allocation in table: %d\n",temp);
	temp=fl[addr].address;
	if (temp<0){
		printf("File does not exist\n");
	}
	else{
		unassignBlocks(temp);	
	}
	/*printf("befname%s\n",fl[addr].name);
	printf("befname%d\n",fl[addr].size);
	printf("befname%d\n",fl[addr].address);
	printf("TEMP%d\n",addr);*/
	Sblock->files-=1;
	fl[addr].size='\0';
	fl[addr].address=0;
	strcpy(fl[addr].name,"");
	
	/*printf("name%s\n",fl[addr].name);
	printf("name%d\n",fl[addr].size);
	printf("name%d\n",fl[addr].address);*/
}
void copytoMinix(char * file){
	


	int fd;
	int result;
	char str[80];
	int temp;
	printf("FLAGGLAL\n");
	temp=findAllocationbyName(file);
	printf("File: %s, size: %d\n",fl[temp].name,fl[temp].size);
	strcpy(str,MINIXPATH);
	strcat(str,file);
	printf("\nCopying to... %s\n",str);
	fd = open(str, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
	if (fd == -1) {
		perror("Error opening file for writing");
		/*return 1;*/
	}
	result = lseek(fd, fl[temp].size, SEEK_SET);
	if (result == -1) {
		close(fd);
		perror("Error calling lseek() to 'stretch' the file");
		/*return 1;*/
	}

	/* write just one byte at the end */
	result = write(fd, "", 1);
	if (result < 0) {
		close(fd);
		perror("Error writing a byte at the end of the file");
		/*return 1;*/
	}

	/* do other things here */

	close(fd);


}
char* generName(void){
	char * file;
	char temp[15];
	strcpy(file,"test");
	sprintf(temp,"%d",(rand() % (10 - 1 + 1)) + 1);
	strcat(file,temp);/*(rand() % (max - min + 1)) + min*/
	strcat(file,".y");
	return file;
}
void createfileVD(char * file){

	
	int blocks;
	int size;
	int pos;
	int val;
	printf("Creating... %s\n",file);
	val=findAllocationbyName(file);
	/*printf("VALVALVALVAL %d\n",val);*/
	if(val == -1 ) {
		/*printf("File size: %ld\n",buffer.st_size);*/
		size=randomSize();
		blocks=calculateBlocks(size);
		if (blocks<Sblock->freeblocks){
			printf("File: %s\tsize: %d\trequires: %d blocks\n",file,size,blocks);
			/*printf("STRUCTURE FILES BEFORE ASSIGN BLOCKS:\n");*/
			/*printf("%d\n",fl[Sblock->files-1].address);*/
			pos=assignBlocks(blocks);
			printf("Address in file table: %d\t",pos+1);
			printf("Address in disk: %d\n\n",fl[pos].address);/*Assign values in File Table*/
			strcpy(fl[pos].name,file);
			fl[pos].size=size;
		}
		else{
			perror("Not enough free space in disk\n");
		}
	}
	else{
		if (val >= 0){printf("File: %s, already exists\n",file);}
		else {printf("File: %s, does not exist\n",file);}
	}	
}
void automaticfilevd(void){
	createfileVD(generName());
}
int main(void)
{
	int temp;
	char * temps;
	srand(time(0));
	createVD(4);/*Size in Kilobytes*/
	/*createList(); already created since createVD()*/
	Sblockchars();
	/*temp=sizeof(Sblock);
	printf("Size of file table: %d\n",temp);*/
	/*createmanualFile();*/

	
	createfileVD("first.y");
	createfileVD("second.y");
	createfileVD("test1.y");
	createfileVD("test2.y");
	createfileVD("test3.y");
	createfileVD("test4.y");
	createfileVD("test5.y");
	createfileVD("test6.y");
	createfileVD("test8.y");
	createfileVD("test9.y");
	createfileVD("test10.y");
	createfileVD("test11.y");
	createfileVD("test12.y");
	createfileVD("test13.y");
	createfileVD("test14.y");
	createfileVD("test15.y");
	createfileVD("test16.y");
	createfileVD("test17.y");
	/*automaticfilevd();*/
	copytoMinix("first.y");
	copyfromMinix("testa.x");
	copyfromMinix("testb.x");
	copyfromMinix("testc.x");
	copyfromMinix("testd.x");
	copyfromMinix("teste.x");
	copyfromMinix("testf.x");
	copyfromMinix("testg.x");
	copyfromMinix("testh.x");
	copyfromMinix("testi.x");
	copyfromMinix("testj.x");
	copyfromMinix("testk.x");
	copyfromMinix("testl.x");
	copytoMinix("test1.y");
	deletefile("second.y");
	
	mapVD();
	exploreVDFiles();
	Sblockchars();
	
	mapVD();
	exploreVDFiles();
	Sblockchars();

	generName();
	return 0;
}
