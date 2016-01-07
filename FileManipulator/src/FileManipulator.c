/*
 * file_manipulator.c
 *
 *  Created on: Jan 26, 2015
 *      Author: Saumitra Aditya
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define INSTR_LENGTH 200
#define COMMAND_LENGTH 10
#define MAX_NUM_ARGS 11 //Instructions can have maximum of 11 arguments

/*declare global variables*/
char *instrFile ;
char *inputFile ;
char *outputFile ;
int blockSize;
int buffSize;
//buffer to hold contents while manipulating.
 typedef struct Buffer
{
	char *head;
	int startBlock;
	int emptyFlag;
}buffer;

buffer *buff;
char *backupFile = "backingStore.txt";

/*structure to hold arguments from tokenized instruction*/
//array to store arguments from the instruction
char InstrLine[INSTR_LENGTH];
char *args[MAX_NUM_ARGS];

/* Breaks the string pointed by str into words and stores them in the arg array*/
int getArguments(char *,char *[]);
/*reads instructions from the file executes them*/
void ExecuteInstructions(char *filename);
/*executes individual instruction*/
void execute(char *);
/*copy contents of one file to another*/
void copyFile(char *,char *);
/*write a block into a file from buffer*/
void updateFile(char *, int,int,int);
/*reads blocks into the buffer from file*/
void readFile(char *, int);

int main (int argc, char **argv)
{
	instrFile = argv[1];
	inputFile = argv[2];
	outputFile = argv[3];
	blockSize = atoi(argv[4]);
	buffSize = atoi(argv[5]);
	printf("\n Arguments provided are \ninstrFile: %s, \ninputFile: %s, \noutputFile:%s",instrFile,inputFile,outputFile);
	printf("\n blockSize: %d, buffSize: %d",blockSize,buffSize);
	//allocate space to buffer based on the provided arguments.
	buff = ( buffer *)malloc(sizeof(buffer));
	buff->head = (char *)malloc(sizeof(char)*buffSize*blockSize);
	buff->startBlock = 0;
	buff->emptyFlag = 0;
	/*create a temp file to be used as backing store when buffer becomes full
	 * Initalize it by copying input file into it
	 * at any pint of time it will contain the latest copy
	 * of block after buffer*/
	copyFile(inputFile,backupFile);
	copyFile(inputFile,outputFile);
	/*Reads the instruction file and executes instructions*/
	ExecuteInstructions(instrFile);
	return 0;
}
/*reads instructions from the file and store them to a string array*/
void ExecuteInstructions(char *filename)
{
	FILE * file;
	char line[128];

	file = fopen(filename, "r");
	if (file == NULL)
		{
			perror("\n Error while reading instruction file.");
			exit(EXIT_FAILURE);
		}
	char *temp = NULL;
	while ((temp = fgets(line,sizeof(line),file))!= NULL)
	{
		//execute each instruction
		execute(temp);
		//printf("\n %s\n",temp);

		/*len = strlen(line);
		line[len-1] = '\0';
		buf = malloc(sizeof(strlen(line)));
		strcpy(buf,line);
		instrArr[i] = buf;
		i++;*/
	}
   fclose(file);

}

/* Breaks the string pointed by str into words and stores them in the arg array*/
int getArguments(char *str, char *arg[])
{
     char delimeter[] = " ";
     char *temp = NULL;
     int i=0;
     temp = strtok(str,delimeter);
     while (temp != NULL)
     {
           arg[i++] = temp;
           temp = strtok(NULL,delimeter);
     }
     arg[i] = NULL;
     return i;
}
/*converts char to binary*/
char *chartobin ( unsigned char c )
{
    /*static char bin[CHAR_BIT + 1] = {0};
    int i;
    for( i = CHAR_BIT - 1; i >= 0; i-- )
    {
        bin[i] = (c % 2) + '0';
        c /= 2;
    }*/
   return "";
}
/*executes a instruction*/
void execute(char *instr)
{


	/* Breaks the command line into arguments and stores the arguments in args array */
	int nargs = getArguments(instr,args);
	int inBlock = atoi(args[1]);
	int outBlock = atoi(args[2]);
	int i =0;
	printf("\n");
	for (i = 0;i<nargs;i++)
	{
		printf("%s\t",args[i]);
	}
	/*if a revert operation
	 * create a suitable mask*/
	char mask;
	if (strcmp(args[0],"revert") == 0)
	{
		mask = 1 << atoi(args[3]);
		for (i = 4;i<nargs;i++)
		{
			mask = mask | (1<< atoi(args[i]));
		}
		printf("\nMask for revert instr is: %s\n",chartobin(mask));
		/*if buffer is empty read from backingStore*/
		if (buff->emptyFlag == 0)
		{
			//read from backing store
			readFile(backupFile,inBlock);
			//input block in buffer.
			//move offset to required position in buffer and apply mask to it.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]^=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				//printf("%c",buff->head[i]);
				printf(" %d%c%s%c%c",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
			}
			printf("\n");
			printf("%s\t%s",chartobin(buff->head[0]),chartobin(buff->head[99]));
			printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
		/*if block in buffer ,perform processing in buffer itself write to output file*/
		else if ((inBlock < (buff->startBlock + buffSize)) && (inBlock>= buff->startBlock) )
		{
			//input block in buffer.
			//move offset to required position in buffer and apply mask to it.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]^=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				//printf("%c",buff->head[i]);
				printf(" %d%c%s%c%c\n",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
			}
			//printf("\n");
			printf("%s-%s%c\n",chartobin(buff->head[0]),chartobin(buff->head[99]),buff->head[99]);
			//printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
		/*if block not in buffer and buffer full
		 * flush buffer to backing store
		 * load buffer with requested block range
		 * perform processing in the buffer
		 * update output file */
		else
		{
			//flush buffer
			updateFile(backupFile,buff->startBlock,buff->startBlock,buffSize);
			//read required blockrange into buffer
			readFile(backupFile,inBlock);
			//now since the block is in buffer, process it and write to output file.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]^=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				printf("%c",buff->head[i]);
			}
			printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
	}
	/*Zero operation*/
	else if (strcmp(args[0],"zero") == 0)
	{
		mask = 1 << atoi(args[3]);
		for (i = 4;i<nargs;i++)
		{
			mask = mask | (1<< atoi(args[i]));
		}
		mask = ~(mask);
		printf("\nMask for zero instr is: %s\n",chartobin(mask));
		if (buff->emptyFlag == 0)
		{
			//read from backing store
			readFile(backupFile,inBlock);
			//input block in buffer.
			//move offset to required position in buffer and apply mask to it.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]&=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				//printf("%c",buff->head[i]);
				printf(" %d%c%s%c%c",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
			}
			printf("\n");
			printf("%s\t%s",chartobin(buff->head[0]),chartobin(buff->head[99]));
			printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
		/*if block in buffer ,perform processing in buffer itself write to output file*/
		else if ((inBlock < (buff->startBlock + buffSize)) && (inBlock>= buff->startBlock) )
		{
			//input block in buffer.
			//move offset to required position in buffer and apply mask to it.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]&=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				//printf("%c",buff->head[i]);
				printf(" %d%c%s%c%c\n",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
			}
			//printf("\n");
			printf("%s-%s%c\n",chartobin(buff->head[0]),chartobin(buff->head[99]),buff->head[99]);
			//printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
		/*if block not in buffer and buffer full
		 * flush buffer to backing store
		 * load buffer with requested block range
		 * perform processing in the buffer
		 * update output file */
		else
		{
			//flush buffer
			updateFile(backupFile,buff->startBlock,buff->startBlock,buffSize);
			//read required blockrange into buffer
			readFile(backupFile,inBlock);
			//now since the block is in buffer, process it and write to output file.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]&=mask);
			}
			printf("\n");
			for (i = 0;i< buffSize*blockSize;i++)
			{
				printf("%c",buff->head[i]);
			}
			printf("\n");
			//write it to outputfile.
			updateFile(outputFile,inBlock,outBlock,1);
		}
	}
	else
	{
		perror("invalid instruction");
	}

}

/*copy contents of one file to another*/
void copyFile(char *src, char *dst)
{
	int ifd,ofd;
	char buff[1024];
	int SrcBlockNum = 0;
	int Length = 0;
	ifd = open(src,O_RDONLY);
	if (ifd == -1)
	{
		perror(" error opening inputfile");
	}
	ofd = open(dst, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG);
	if (ofd == -1)
	{
		perror(" error opening dstfile");
	}
	ssize_t rcnt = pread(ifd, buff, 1024, SrcBlockNum*1024);
	while (rcnt > 0){
		Length = rcnt;
		int wcnt = pwrite(ofd, buff, Length, SrcBlockNum*1024);
		if (wcnt == -1) perror ("Failed to write data block to output file");
		SrcBlockNum++;
		rcnt = pread(ifd, buff, 1024, SrcBlockNum*1024);
	}

	return;
	/*FILE *fp1,*fp2;
	fp1 = fopen(src,"r");
	char ch;
	if (fp1==NULL)
	{
		perror("\n Cannot open src file for copying.\n");
		exit(1);
	}
	fp2 = fopen(dst,"w");
	if (fp2==NULL)
	{
		perror("\n Cannot open dst file for copying.\n");
		fclose(fp1);
		exit(1);
	}
	while((ch=getc(fp1))!=EOF)
	      putc(ch,fp2);
	  printf("\n copied input file to backing store\n");
	  fclose(fp1);
	  fclose(fp2);*/
}

/*write a block into a file from buffer*/
void updateFile(char *dst, int inblock,int outblock,int numBlocks)
{
	int fd1 = open(dst, O_CREAT|O_WRONLY,  S_IRWXU|S_IRWXG);
	if (fd1== -1)
	{
		perror("\n Cannot write block into file from buffer.\n");
		exit(1);
	}
	pwrite(fd1,buff->head + (inblock - buff->startBlock)*blockSize,(size_t)(blockSize * sizeof(char)* numBlocks),(off_t)((outblock)*blockSize));
	close(fd1);
	/*FILE *fp1 = fopen(dst,"r+b");
	if (fp1==NULL)
	{
		perror("\n Cannot write block into file from buffer.\n");
		exit(1);
	}
	//move offset in dst file to required position
	fseek(fp1,(outblock-1)*blockSize,SEEK_SET);
	//fwrite("12345678",sizeof(char),8,fp1);
	fwrite(buff->head + (inblock - buff->startBlock)*blockSize, blockSize * sizeof(char),numBlocks,fp1);
	fflush(fp1);*/
}

/*reads blocks into the buffer from file*/
void readFile(char *src, int block)
{
	int fd1 = open(src,O_RDONLY);
	if (fd1== -1)
	{
		perror(" Cannot read blocks into buffer");
		exit(1);
	}
	pread(fd1,buff->head,(size_t)(blockSize * sizeof(char)*buffSize),(off_t)((block)*blockSize));
	buff->startBlock = block;
	buff->emptyFlag = 1;
	close(fd1);

	/*FILE *fp1 = fopen(src,"r");
	if (fp1==NULL)
	{
		perror("\n Cannot read blocks into buffer.\n");
		exit(1);
	}
	fseek(fp1,(block-1)*blockSize,SEEK_SET);
	fread(buff->head,blockSize * sizeof(char),buffSize,fp1);
	buff->startBlock = block;
	buff->emptyFlag = 1;*/
}
