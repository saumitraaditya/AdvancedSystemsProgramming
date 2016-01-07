/*
Usage
To compile: gcc twochildshell.c -o twochildshell.o
To execute: ./twochildshell.o

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#define COMMAND_LINE_LENGTH 200
#define COMMAND_LENGTH 10
#define MAX_NUM_ARGS 10

/* Breaks the string pointed by str into words and stores them in the arg array*/
int getArguments(char *,char *[]);

/* Reads a line and discards the end of line character */
int getLine(char *str,int max);

/* Recieves the tokenized array and checks for presence of pipe "|" or redirection ">" operator 
   returns 0 if no pipe or redirection
   returns 3 if both pipe and redirection
   returns 1 if only pipe 
   returns 2 if only redirection*/
int typeChecker(char *[]);

/* execute commands */
int execute (char *[]);

/*execute pipe cmd1 */
void runCmd1(int [], char *[],char *);

/*execute pipe cmd2 */
void runCmd2(int [], char *[],char *);

/* Simulates a shell, i.e., gets command names and executes them */
int main(int argc, char *argv[])
{

  char commandLine[COMMAND_LINE_LENGTH+1]; 
  char *arguments[MAX_NUM_ARGS];

  
  printf("Welcome to Extremely Simple Shell\n");

  int exit= 0;
  do { 
      int status;
      char *temp;
      
      /* Prints the command prompt */
      printf("\n$ ");
      
      /* Reads the command line and stores it in array commandLine */
      getLine(commandLine,COMMAND_LINE_LENGTH+1);

      /* The user did not enter any commands */
      if (!strcmp(commandLine,""))
         continue;
      
      /* Breaks the command line into arguments and stores the arguments in arguments array */
      int numArgs = getArguments(commandLine,arguments);
      
      /* Terminates the program when the user types exit or quit */
      if (!strcmp(arguments[0],"quit") || !strcmp(arguments[0],"exit"))
         break;
         
      /* Creates a child process */   
      int pid = fork();
      
      switch (pid)
      {
          case 0: /* Executed by the child process only */ 
                  execute(arguments);// handles commands to be executed.
                  return 1;
          case -1: /* Executed only when the fork system call fails */
                  perror("Fork error\n");
                  break;
          default: /* Executed by parent process only  */
                  wait(&status);
                  break;        
      }
  }
  while (1); /* Use int values to represent boolean in C language: 0 denotes false, non-zero denotes true */


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

/* Reads a line and discards the end of line character */
int getLine(char *str,int max)
{
    char *temp;
    if ((temp = fgets(str,max,stdin)) != NULL)
    {
       int len = strlen(str);
       str[len-1] = '\0';
       return len-1;
    }   
    else return 0;
}

/* Recieves the tokenized array and checks for presence of pipe "|" or redirection ">" operator 
   returns 0 if no pipe or redirection
   returns 3 if both pipe and redirection
   returns 1 if only pipe 
   returns 2 if only redirection*/

int typeChecker(char **argv)
{
    int pipe_flag = 0;
    int redir_flag = 0;
    int i = 0;
    while ( argv[i] != NULL)
        {
            if (!strcmp( argv[i],"|"))
            pipe_flag = 1;
            if (!strcmp( argv[i],">"))
            redir_flag = 1;
            i++;
        }
    if (pipe_flag == 0 && redir_flag == 0)
        return 0;
    else if (pipe_flag ==1 && redir_flag == 1)
        return 3;
    else if (pipe_flag == 1)
        return 1;
    else
        return 2;
}

/* executes the operations specified as input to shell from stdin
 depending on requirement spawns child processes to execute individual commands
 handles pipe and output redirection functionality.*/
int execute (char **argv)
{
    int type = typeChecker(argv); //detect presence of pipe/redirection operator.
    char *cmd1[MAX_NUM_ARGS]; // holds command1 and its arguments i.e. command to left of pipe
    char *cmd2[MAX_NUM_ARGS]; // command to right of pipe 
    char redir[40]; // name of file to which output is to be redirected.
    int pos_pipe = 0;//position of pipe in tokenized array
    int pos_redir = 0;//position of redirection operator
    int pos = 0;
    int arg_size=0;//number of tokens in entered command
    int i = 0;
    int j = 0;
    int p[2]; //array to hold file descriptors from pipe
    int status;
    int pid;
    switch (type)
    {
        case 0: //no pipe or redirection.
            execvp(argv[0],argv);
            perror("Exec error\n");
            return 1;
            
        case 1: // only pipe 
            //find position of pipe 
            while (argv[pos]!= NULL)
            {
                if (!strcmp( argv[pos],"|"))
                    pos_pipe = pos;
                pos++;
            }
            arg_size = pos;
            // copy command and arguments for command1
            
            for (  i = 0;i < pos_pipe;i++)
            {
                cmd1[i] = argv[i];
            }
            cmd1[i] = NULL;
            
            //copy commands and arguments for command2
             j = 0;
            for ( i = pos_pipe+1;i <= arg_size;i++)
            {
                cmd2[j++] = argv[i];
            }
            cmd2[j] = NULL;
            //print contents of command1
            /*i = 0;
            while (cmd1[i] != NULL)
            {
                //printf("%s\n",cmd1[i]);
                i++;
            }
            //print contents of command2
            i = 0;
            //printf(" cmd2 is %s\n",cmd2[0]);
            while (cmd2[i] != NULL)
            {
                //printf("%s\n",cmd2[i]);
                i++;
            }*/
            
            // implement the piping mechansim
            pipe(p);
            
            runCmd1(p,cmd1,NULL);//execute command1, handles filedescriptors
            runCmd2(p,cmd2,NULL);//execute command2
            
            //close both ends of pipe in parent as it does not uses them.
            close(p[0]);
            close(p[1]);
            // wait for all child processes to finish
            while ((pid = wait(&status)) != -1)	
                continue;
		   
            return 1;
            
        case 2:// only redirection
        //find position of redirection operator
            while (argv[pos]!= NULL)
            {
                if (!strcmp( argv[pos],">"))
                    pos_redir = pos;
                pos++;
            }
            arg_size = pos;
            // copy command and arguments for command1
            for (  i = 0;i < pos_redir;i++)
            {
                cmd1[i] = argv[i];
            }
            cmd1[i] = NULL;
            //file to which output is to be redirected is argv[pos_redir +1]
            close(1);//close the stdout for process
            open(argv[pos_redir+1],O_CREAT | O_RDWR |O_APPEND ,0777);//open file in required mode to redirect output,will fail if file already exists.
            execvp(cmd1[0],cmd1);
            perror("Exec error\n");
            
        case 3: //both pipe and redirection operator present in the command
                // assumes pipe comes before redirection
               
           while (argv[pos]!= NULL)
            {
                if (!strcmp( argv[pos],">"))
                    pos_redir = pos;
                if (!strcmp( argv[pos],"|"))
                    pos_pipe = pos;
                pos++;
            }
            arg_size = pos;
            // copy command1 and arguments for command1
            for (  i = 0;i < pos_pipe;i++)
            {
                cmd1[i] = argv[i];
            }
            cmd1[i] = NULL;
           
            // copy command2 and its arguments
            j = 0;
            for (  i = pos_pipe+1;i < pos_redir;i++)
            {
                cmd2[j++] = argv[i];
            }
            cmd2[j] = NULL;
           
            // make note of the file into which output is to be re-directed
            strcpy(redir,argv[pos_redir+1]);
            
            pipe(p);
            
            runCmd1(p,cmd1,NULL);
            runCmd2(p,cmd2,redir);//pass the pointer to filename to which o/p is to be redirected.
            
            close(p[0]);
            close(p[1]);
            //wait for all child processes to finish
            while ((pid = wait(&status)) != -1)	
                continue;
		   
            return 1;
            
        default:
            break;   
    }
}

/*Runs individual commands handles filedescriptors for i/o redirections*/

// executes command to left of pipe
void runCmd1(int fd[], char **cmd1,char *redir)
{
    int pid;
    switch (pid = fork())
    {
        case 0://child
            dup2(fd[1],1);//close stdout
            close(fd[0]);//close read end of pipe, not used
            execvp(cmd1[0],cmd1);
            perror(cmd1[0]);
            break;
        default: //do nothing if parent
            break;
    }
    
}
//executes command to right of pipe, if redirection file is provided closes
// stdout.
void runCmd2(int fd[], char **cmd2,char *redir)
{
    int pid;
    switch (pid = fork())
    {
        case 0://child
            dup2(fd[0],0); //close stdin,use pipe read-end
            close(fd[1]); //write end of pipe not used
            if (redir != NULL)// if redirection to a file required
            {
                close(1);//close the stdout for process
                open(redir,O_CREAT | O_RDWR |O_APPEND ,0777); //will fail if file already exists.
            }
            execvp(cmd2[0],cmd2);
            perror(cmd2[0]);
            break;
        default: //if parent do nothing.
            break;
    }
    
}
