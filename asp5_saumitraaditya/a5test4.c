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
*
*   Use root to run this code, initially in Mode2.
*   Scenario:-
*   Thread1                     Thread2
*   open()--------------------------------------------------increments count2
*                               open()----------------------increments count2
*   
*   pthread_exit()------------------------------------ Dies without decrementing count2
*                               ioctl(mode1)---------------- goes into wait as count2>1
*                                                            
*   In this situation although other threads can access device do reads/writes etc, they will never be able to 
*   change the mode, as count2 can never be brought to one as Thread1 died before it could decrement count2.                            
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

    sleep(10);
    
    
   //killing this thread
    printf("killing thread handling seq1\n");
    pthread_exit(NULL);
    
    //close device
    close(fd);
    printf("closed device in seq1\n");
}

void *seq2(void *arg)
{
    //simply open the device, it will block while trying to acquire sem2.
    //int fd = open(DEVICE, O_RDWR);
    
    printf("calling open in seq2\n");
    int fd = open(DEVICE, O_RDWR);
    printf("opened file in seq2\n");
   
    sleep(10);
    
    
    printf("calling ioctl in seq2\n");
    int rc = ioctl(fd, E2_IOCMODE1, 1);
    if (rc == -1)
    	{ 
    	    perror("\n***error in ioctl in seq1***\n"); 
	    }
    printf("done ioctl in seq2\n");
	    
    
    close(fd);
    printf("closed device in seq2\n");
    
}

int main(int argc, char *argv[]) 
{

    /*
        1. We will run two threads running two sequences of operations.
    */
    
    //change mode to 2 and than close file
    int fdm = open(DEVICE, O_RDWR);
    int rc = ioctl(fdm, E2_IOCMODE2, 2);
    if (rc == -1)
    	{ 
    	    perror("\n***error in ioctl in main***\n"); 
	    }
    printf("done ioctl in main\n");
    close(fdm);
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
