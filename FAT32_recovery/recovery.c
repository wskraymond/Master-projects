#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include <sys/types.h>

#pragma pack(push,1)
struct BootEntry
{
	unsigned char BS_jmpBoot[3];	/* Assembly instruction to jump to boot code */
	unsigned char BS_OEMName[8];	/* OEM Name in ASCII */
	unsigned short BPB_BytsPerSec; /* Bytes per sector. Allowed values include 512, 1024, 2048, and 4096 */
	unsigned char BPB_SecPerClus; /* Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller */
	unsigned short BPB_RsvdSecCnt;	/* Size in sectors of the reserved area */
	unsigned char BPB_NumFATs;	/* Number of FATs */
	unsigned short BPB_RootEntCnt; /* Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32 */
	unsigned short BPB_TotSec16;	/* 16-bit value of number of sectors in file system */
	unsigned char BPB_Media;	/* Media type */
	unsigned short BPB_FATSz16; /* 16-bit size in sectors of each FAT for FAT12 and FAT16.  For FAT32, this field is 0 */
	unsigned short BPB_SecPerTrk;	/* Sectors per track of storage device */
	unsigned short BPB_NumHeads;	/* Number of heads in storage device */
	unsigned long BPB_HiddSec;	/* Number of sectors before the start of partition */
	unsigned long BPB_TotSec32; /* 32-bit value of number of sectors in file system.  Either this value or the 16-bit value above must be 0 */
	unsigned long BPB_FATSz32;	/* 32-bit size in sectors of one FAT */
	unsigned short BPB_ExtFlags;	/* A flag for FAT */
	unsigned short BPB_FSVer;	/* The major and minor version number */
	unsigned long BPB_RootClus;	/* Cluster where the root directory can be found */
	unsigned short BPB_FSInfo;	/* Sector where FSINFO structure can be found */
	unsigned short BPB_BkBootSec;	/* Sector where backup copy of boot sector is located */
	unsigned char BPB_Reserved[12];	/* Reserved */
	unsigned char BS_DrvNum;	/* BIOS INT13h drive number */
	unsigned char BS_Reserved1;	/* Not used */
	unsigned char BS_BootSig; /* Extended boot signature to identify if the next three values are valid */
	unsigned long BS_VolID;	/* Volume serial number */
	unsigned char BS_VolLab[11]; /* Volume label in ASCII. User defines when creating the file system */
	unsigned char BS_FilSysType[8];	/* File system type label in ASCII */
};
#pragma pack(pop)

#pragma pack(push,1)
struct DirEntry
{
	    unsigned char DIR_Name[11];		/* File name */
	    unsigned char DIR_Attr;		/* File attributes */
	    unsigned char DIR_NTRes;		/* Reserved */
	    unsigned char DIR_CrtTimeTenth;	/* Created time (tenths of second) */
	    unsigned short DIR_CrtTime;		/* Created time (hours, minutes, seconds) */
	    unsigned short DIR_CrtDate;		/* Created day */
	    unsigned short DIR_LstAccDate;	/* Accessed day */
	    unsigned short DIR_FstClusHI;	/* High 2 bytes of the first cluster address */
	    unsigned short DIR_WrtTime;		/* Written time (hours, minutes, seconds */
	    unsigned short DIR_WrtDate;		/* Written day */
	    unsigned short DIR_FstClusLO;	/* Low 2 bytes of the first cluster address */
	    unsigned long DIR_FileSize;		/* File size in bytes. (0 for directories) */
};
#pragma pack(pop)

//global variable
struct BootEntry BSI;
struct DirEntry entry;	
char* dev_fname;
int fd;

// load information from boot sector of device file
int open_read_BS(char* dname){

	if((fd=open(dname,O_RDONLY))==-1)
	{
		printf("Unable to open disk %s\n",dname);
		return 0;
	}
	else{
		if(read(fd,&BSI,sizeof(struct BootEntry))==-1)
		{	
			printf("Unable to read the device file\n");
			close(fd);
			return 0;
		}
	    }
	return 1;
}

//Show tips for invalid arguments
void usage(char str[])
{	
	printf("\nUsage: %s -d [device filename] [other arguments]", str);
	printf("\n-i                    Print boot sector information");
	printf("\n-l                    List the root directory entries");
	printf("\n-r filename  	      File recovery\n");
	exit(1);
}

// opern device file and read it
void bs_Info(){
	printf("Number of FATs = %d\n", BSI.BPB_NumFATs);
	printf("Number of bytes per sector = %d\n", BSI.BPB_BytsPerSec);
	printf("Number of sectors per cluster = %d\n", BSI.BPB_SecPerClus);
	printf("Number of reserved sectors = %d\n", BSI.BPB_RsvdSecCnt);
}

int locateFAT() //locate FAT
{
	lseek(fd,BSI.BPB_RsvdSecCnt*BSI.BPB_BytsPerSec,SEEK_SET);	
}

int isEnd(unsigned int index) 
{
	if(index>=0xFFFFFF8 && index<=0xFFFFFFF)  // range of EOC
		return 1;
	return 0; 
}

int nextClusterIndex(unsigned int index)
{		
		unsigned int nextIndex;		
		locateFAT(); //locate FAT
		lseek(fd, 4*index, SEEK_CUR);  
		read(fd, &nextIndex, 4);
		return nextIndex;
}

void locateCluster(unsigned int index)  //if index start from 2.
{
	int i;
	locateFAT();
	lseek(fd, BSI.BPB_NumFATs * BSI.BPB_FATSz32 * BSI.BPB_BytsPerSec, SEEK_SET);
	lseek(fd, (BSI.BPB_RootClus-2) * BSI.BPB_SecPerClus * BSI.BPB_BytsPerSec, SEEK_CUR);
	for(i = 2; i < index; i++) //if index is 2, do nothing, becuase the origin is at cluster 2.
		lseek(fd,BSI.BPB_BytsPerSec * BSI.BPB_SecPerClus,SEEK_CUR);
}

char pfilename[12]; //used to output
char* printable_filename(struct DirEntry target) 
{
	int i;
	target.DIR_Name[i];

	for(i = 0; target.DIR_Name[i]!=' ' && i<8; i++)
		pfilename[i] = temp[i];

	/* If containing extension */
	if( target.DIR_Name[8] != ' ' )
	{
		pfilename[i++] = '.';
		pfilename[i++] = temp[8];
		pfilename[i++] = temp[9];
		pfilename[i++] = temp[10];
	}
	/* If: is a directory */
	if ( target.DIR_Attr&0x10 )
		pfilename[i++] = '/';
	pfilename[i] = '\0';

	return &pfilename;
}

int filename_comparison(char* str1, char* str2)
{
	int len1, len2;

	str1++;
	str2++;
	len1 = strlen(str1);
	len2 = strlen(str2);

	if (len1!=len2 || strcasecmp(str1,str2) )
		return 0;
	return 1;
}

int search_delete(char* filename)
{
	unsigned int index = 2;
	locateCluster(index); //start from the index 2;
	int i,read_size;
	
	for(read_size = 0; read_size < BootSector.BPB_SecPerClus*BootSector.BPB_BytsPerSec; read_size += sizeof(struct DirEntry)) // should be less than size of a cluster
	{
		if(strlen(filename)<= 12)  //Not LFN
		{
			if(read(fd,&entry,sizeof(struct DirEntry))==-1)
				printf("unable to read directory entry");
			else if(entry.Dir_Name[0] == 0xe5)
			{
				if(filename_comparison(printable_filename(entry.Dir_Name),filename))
					break;					
			}
		}
		else
		{ 
			//LFN
		}
	}
	nextClusterIndex(index);
}

int recovery()
{
	
}

int main(int argc, char* argv[]){
	int flag =1;  // 1 is invalid;  0 is valid arguments
	char choice=' ';  // -i, -l, -r
	char* fname;

	if(argc>3 && argc<6)
		if(strcmp(argv[1],"-d")==0){
			dev_fname = strdup(argv[2]);

			if(argc==4){
				if(strcmp(argv[3],"-i")==0)
				{	
					flag=0;
					choice='i';
				}
				else if(strcmp(argv[3],"-l")==0)
				{
					flag=0;
					choice='l';
				}}
			else if(argc==5)
				if(strcmp(argv[3],"-r")==0)
				{
					flag =0;
				  	fname = strdup(argv[4]);
					choice = 'r';
				}
		}

	// open and read device file
	if(open_read_BS(dev_fname)==0)
		exit(1);

	// If invalid arguments, print usage ||  Otherwise, excute the command			
	if(flag)
		usage(argv[0]);
	else
	{
		switch(choice){
			case 'i': bs_Info();
				  break;
			case 'l': break;	
			case 'r': break;
		}
	}
	close(fd);
	return 0;
}
