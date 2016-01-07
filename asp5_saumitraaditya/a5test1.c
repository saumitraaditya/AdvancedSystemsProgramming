#include <linux/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)
#define SIZE 10
#define nTHread 2



/*
********************** NOTE ***********************************************************************************
*   CAUTION: REAL DEADLOCK -- Your sytem will hang
*   Use root to run this code, Initially in Mode1.
*   Scenario:-
*   Thread1                     Thread2
*   open()
*   down sem2 ---------------------------------
*                               open()----------------------increments count, blocks on sem2
*   ioctl(mode2)------------------------------------------- goes into wait holding sem2, can be woken up 
*                                                            if count1 = 1, since
*                                                           no thread/process can proceed to release() 
*                                                            to decrement count1 without
*                                                           obtaining sem2, the entire execution 
*                                                            flow goes into deadlock.
*********************** END NOTE *******************************************************************************                                                     
*/
char *DEVICE = "/dev/a5";

pthread_t threads[nTHread];

void *seq1(void *arg)
{
    char buf[SIZE];
    memset(buf, 'A', SIZE);
    printf("calling open in seq1\n");
    int fd = open(DEVICE, O_RDWR);
    printf("opened file in seq1\n");
    
    /*
    FILE *fp;
    printf("calling open in seq1\n");
    fp = fopen(DEVICE,"w+");
    printf("opened file in seq1\n");
    */
    sleep(20);
    
    
    printf("calling ioctl in seq1\n");
    int rc = ioctl(fd, E2_IOCMODE2, 2);
    if (rc == -1)
    	{ 
    	    perror("\n***error in ioctl in seq1***\n"); 
	    }
    printf("done ioctl in seq1\n");
	    
   
    close(fd);
    printf("closed device in seq1\n");
}

void *seq2(void *arg)
{
    //simply open the device, it will block while trying to acquire sem2.
    //int fd = open(DEVICE, O_RDWR);
    
    FILE *fp;
    printf("calling open in seq2\n");
    fp = fopen(DEVICE,"w+");
    printf("opened file in seq2\n");
    fclose(fp);
    printf("closed device in seq2\n");
    
}

int main(int argc, char *argv[]) 
{

    /*
        1. We will run two threads running two sequences of operations.
    */
    
    // create two threads
    pthread_create(&(threads[0]), NULL, seq1, (void*)(0));
    sleep(5);
    pthread_create(&(threads[1]), NULL, seq2, (void*)(1));
    int i;
    // wait for these threads to finish their respective sequences.
    for (i=0; i<nTHread; i++)
            pthread_join(threads[i], NULL);

    return 0;
    
}
