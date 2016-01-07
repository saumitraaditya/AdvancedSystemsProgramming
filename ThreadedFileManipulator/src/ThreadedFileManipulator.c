/*
 ============================================================================
 Name        : ThreadedFileManipulator.c
 Author      : sam
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


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
	int *book; //keeps track of destination blocks.
}buffer;

typedef struct Instructions
{
	char *inst[200];
	int numInstr;
}instructions;
//allocate instructions structure
instructions *instr;

buffer *buff;
char *backupFile = "backingStore.txt";

/*Mutex and conditions*/
pthread_mutex_t mutex;
pthread_cond_t missing_empty; //block not in buffer and buffer empty
pthread_cond_t missing_full; //block not in buffer and buffer full
pthread_cond_t load;//condition for loader to read blocks from input file to buffer
pthread_cond_t flush;//condition for writer to flush buffer into output file and backing-store.
int LOAD = 0;//initially false
int FLUSH = 0;//initially false
int  block_to_load = -1;//this variable tells reader thread which block to load.
int done_load = 0; //to indicate to loading thread that no work is left and it should exit.
int done_flush = 0; //to indicate to flushing thread that no work is left and it should exit.

/*Thread array*/
//pthread_t worker[3];

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

/* Method for processing thread
 * executes instructions one by one
 * blocks if input-block not in buffer*/
void *process(void *tid)
{
	int j;
	for (j=0;j< instr->numInstr;j++)
	{
		pthread_mutex_lock(&mutex);
		execute(instr->inst[j]);
		/*If this is the last instruction time to ask loader and flusher to terminate*/
		if (j == instr->numInstr-1)
		{
			FLUSH = 1;
			block_to_load = -1;
			LOAD = 1;
			done_load = 1;
			done_flush = 1;
			pthread_cond_signal(&flush);
			pthread_cond_signal(&load);
		}
		pthread_mutex_unlock(&mutex);
	}
	printf("\nprocess thread exiting.");
	pthread_exit(NULL);

}

/* Method for reading thread that loads the buffer from backing-store/input file*/
void *loader(void *tid)
{
	int i = 0;
	while(1)
	{
		/*Try to grab the lock*/
		 pthread_mutex_lock(&mutex);
		 printf("\n loader has the lock.");
		 if (done_load == 1)
		 {
			 printf("\n Loader thread received termination order.\n");
			 pthread_mutex_unlock(&mutex);
			 break;
		 }
		 while (LOAD == 0)
		 {
			 printf("\n Loader going to wait.");
			 pthread_cond_wait(&load, &mutex);
		 }
		 if ((block_to_load != -1) && (buff->emptyFlag == 1))
		 {
			 printf("\n loading thread loading blocks starting %d into empty-buffer. \n",block_to_load);
			 pthread_cond_signal(&missing_empty);
			 readFile(backupFile, block_to_load);
			 LOAD = 0;
			 /* Refresh buffer book*/
			 for ( i =0;i<buffSize;i++)
			 {
				 buff->book[i] = -1;
			 }
		 }
		 else if ((block_to_load != -1)&& (buff->emptyFlag == 0))
		 {
			 printf("\n loading thread loading blocks starting %d into full-buffer. \n",block_to_load);
			 pthread_cond_signal(&missing_full);
			 readFile(backupFile, block_to_load);
			 LOAD = 0;
			 /* Refresh buffer book*/
			 for ( i =0;i<buffSize;i++)
			 {
				 buff->book[i] = -1;
			 }
		 }
		 pthread_mutex_unlock(&mutex);

	}
	pthread_exit(NULL);
}

/* Method for writing/flushing thread that loads the buffer from backing-store/input file*/
void *flusher(void *tid)
{
	int i = 0;
	while(1)
	{
		/*Try to grab the lock*/
		 pthread_mutex_lock(&mutex);
		 printf("\n Flusher has the lock.");
		 if (done_flush == 1)
		 {
			 printf("\n FLUSH thread received termination order.\n");
			 //flush buffer into output file for the last operation.
			 for (i = 0;i<buffSize;i++)
			 {
				 /*if this block in buffer has not been modified*/
				 if (buff->book[i] == -1)
				 {
					 continue;
				 }
				 /*if block was modified in buffer write it to respective block in output-file*/
				 else
				 {
					 updateFile(outputFile,buff->startBlock+i,buff->book[i],1);
				 }
			 }
			 pthread_mutex_unlock(&mutex);
			 break;
		 }
		 while (FLUSH == 0)
		 {
			 printf("\nFlusher going to wait");
			 pthread_cond_wait(&flush, &mutex);
		 }
		 printf("\n FLUSH thread updating output file for buffer set starting %d.\n",buff->startBlock);
		 //flush buffer into output file
		 for (i = 0;i<buffSize;i++)
		 {
			 /*if this block in buffer has not been modified*/
			 if (buff->book[i] == -1)
			 {
				 continue;
			 }
			 /*if block was modified in buffer write it to respective block in output-file*/
			 else
			 {
				 updateFile(outputFile,buff->startBlock+i,buff->book[i],1);
			 }
		 }
		 printf("\n FLUSH thread updating backingStore file for buffer set starting %d.\n",buff->startBlock);
		 //flush buffer into backingStore
		 updateFile(backupFile,buff->startBlock,buff->startBlock,buffSize);
		 FLUSH = 0;
		 //signal loading thread to populate the buffer with requested block
		 pthread_cond_signal(&load);
		 LOAD = 1;
		 pthread_mutex_unlock(&mutex);
	}
	pthread_exit(NULL);
}

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
	buff->book = (int *)malloc(sizeof(int)*buffSize);
	buff->startBlock = 0;
	buff->emptyFlag = 1;
	/* Initialize mutex and condition variables*/
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&missing_empty, NULL);
	pthread_cond_init(&missing_full, NULL);
	pthread_cond_init(&load, NULL);
	pthread_cond_init(&flush, NULL);
	/*create a temp file to be used as backing store when buffer becomes full
	 * Initalize it by copying input file into it
	 * at any pint of time it will contain the latest copy
	 * of block after buffer*/
	copyFile(inputFile,backupFile);
	copyFile(inputFile,outputFile);
	/*read instruction file and store instructions in a array*/
    instr = ( instructions *)malloc(sizeof(instructions));
	char buffer[200];
	FILE *ifd = fopen(instrFile,"r");
	if (ifd == NULL)
	{
		perror(" error opening instructionFile");
	}
	char *temp = NULL;
	while ((temp = fgets(buffer,sizeof(buffer),ifd))!= NULL)
	{
		instr->inst[instr->numInstr++] = strdup(buffer);
	}
   fclose(ifd);
   printf("\n Instructions present in instruction file are %d",instr->numInstr);
   int i = 0;
   for (i = 0;i<instr->numInstr;i++)
   {
	   printf("\n%s",instr->inst[i]);
   }

	/*Create threads*/
	pthread_t worker[3];
	int failed;
	failed = pthread_create(&(worker[0]), NULL,process, (void*)(long)0);
	 if (failed)
	 {
	         printf("process thread_create failed!\n");
	         return -1;
	 }
	 failed = pthread_create(&(worker[1]), NULL,loader, (void*)(long)1);
	 if (failed)
	 {
			 printf("loader thread_create failed!\n");
			 return -1;
	 }
	failed = pthread_create(&(worker[2]), NULL,flusher, (void*)(long)2);
	 if (failed)
	 {
		 printf("flusher thread_create failed!\n");
		 return -1;
	 }

	 /*Wait for worker threads to wrap-up*/
	 void *status;
	 for(i=0; i<3; i++)
	   pthread_join(worker[i], &status);

	return 0;
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
	/* grab hold of lock before proceeding further*/
	 //pthread_mutex_lock(&mutex);
	/* if  buffer empty do condition wait
	 * signal loader thread to load the buffer*/
	 while (buff->emptyFlag == 1)
	 {
		 pthread_cond_signal(&load);
		 block_to_load = inBlock;
		 LOAD = 1;
		 pthread_cond_wait(&missing_empty, &mutex);
	 }

	/* if block not in buffer and buffer full*/
	 while ((((inBlock < (buff->startBlock + buffSize)) && (inBlock>= buff->startBlock)) != 1) && (buff->emptyFlag == 0))
	 {
		 pthread_cond_signal(&flush);
		 block_to_load = inBlock;
		 FLUSH = 1;
		 pthread_cond_wait(&missing_full, &mutex);
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

		/*if block in buffer ,perform processing in buffer itself write to output file*/
		if ((inBlock < (buff->startBlock + buffSize)) && (inBlock>= buff->startBlock) )
			{
				//input block in buffer.
				//move offset to required position in buffer and apply mask to it.
				for (i = 0;i< blockSize;i++)
				{
					(buff->head[(inBlock - buff->startBlock)*blockSize+i]^=mask);
				}
				printf("\n");

				/*set book keeping for block*/
				buff->book[inBlock - buff->startBlock] = outBlock;
				/*for (i = 0;i< buffSize*blockSize;i++)
				{
					//printf("%c",buff->head[i]);
					printf(" %d%c%s%c%c\n",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
				}*/
				//printf("\n");
				printf("%s-%s%c\n",chartobin(buff->head[0]),chartobin(buff->head[99]),buff->head[99]);
				//printf("\n");
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

		/*if block in buffer ,perform processing in buffer itself write to output file*/
		if ((inBlock < (buff->startBlock + buffSize)) && (inBlock>= buff->startBlock) )
		{
			//input block in buffer.
			//move offset to required position in buffer and apply mask to it.
			for (i = 0;i< blockSize;i++)
			{
				(buff->head[(inBlock - buff->startBlock)*blockSize+i]&=mask);
			}

			/*set book keeping for block*/
			buff->book[inBlock - buff->startBlock] = outBlock;
			printf("\n");
			/*for (i = 0;i< buffSize*blockSize;i++)
			{
				//printf("%c",buff->head[i]);
				printf(" %d%c%s%c%c\n",i,'-',chartobin(buff->head[i]),'-',buff->head[i]);
			}*/
			//printf("\n");
			printf("%s-%s%c\n",chartobin(buff->head[0]),chartobin(buff->head[99]),buff->head[99]);
			//printf("\n");
		}
	}
	else
	{
		perror("invalid instruction");
	}
	/*release the mutex after executing instruction*/
	//pthread_mutex_unlock(&mutex);

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
	buff->emptyFlag = 0;//sets buffer non-empty.
	close(fd1);
}
