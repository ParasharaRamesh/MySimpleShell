#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/resource.h>
#include<pthread.h>
#include<string.h>
#include<sys/wait.h>
#include "util.h"
#include <time.h>
#include<dirent.h>

#define RECENTCOMMANDS 25
#define ALIASCOUNT  100

typedef struct log{
  char command[30];
  time_t timestamp;
  int processId;
  int bit;
}log;

int currentLogIndex = 0;
log logHistory[RECENTCOMMANDS];

void addLogRecord(char *command, time_t timestamp, int processId)
{
  if( currentLogIndex == 25 )
  {
      currentLogIndex = 0;
  }
  memset(logHistory[currentLogIndex].command,0,sizeof(logHistory[currentLogIndex].command));
  strcpy(logHistory[currentLogIndex].command,command);
  logHistory[currentLogIndex].timestamp = timestamp;
  logHistory[currentLogIndex].processId = processId;
  logHistory[currentLogIndex].bit = 1;
  currentLogIndex++;
}


typedef struct alias{
  char aliascommand[10];
  char originalcommand[10];
  //struct alias *next;
}alias;

int aliasno=0;
alias aliasHistory[ALIASCOUNT];
char cwd[1024];

int currentindex=0;

void init(void)
{
  for(int i=0;i<RECENTCOMMANDS;i++)
  {
      logHistory[i].bit=0;
  }
  for(int i=0;i<ALIASCOUNT;i++)
  {
    aliasHistory[i].aliascommand[0]="\0";
    aliasHistory[i].originalcommand[0]="\0";
  }
}

// function for finding pipe
int parsePipe(char* str, char** strpiped)
{
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }

    if (strpiped[1] == NULL)
        return 0; // returns zero if no pipe is found.
    else {
        return 1;
    }
}

void execArgsPiped(char** parsed, char** parsedpipe)
{
    // 0 is read end, 1 is write end
    int pipefd[2];
    pid_t p1, p2,wpid;

    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be initialized");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }

    if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            printf("\nCould not execute command 1..");
            exit(0);
        }
    } else {
        // Parent executing
        p2 = fork();

        if (p2 < 0) {
            printf("\nCould not fork");
            return;
        }

        // Child 2 executing..
        // It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
	    dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("\nCould not execute command 2..");
                exit(0);
            }
        } 
	else {
            // parent executing, waiting for two children
        //  waitpid(p1,NULL,WNOHANG);
	//waitpid(p2,NULL,WNOHANG);
	wait(NULL);
    }
}
//return;
}
void parseSpace(char* str, char** parsed)
{
    int i;

    for (i = 0; i < 100; i++) {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

void logalias(char * original, char * alias)
{
  strcpy(aliasHistory[aliasno].aliascommand,alias);
  strcpy(aliasHistory[aliasno].originalcommand,original);
  aliasno++;
}

char * getOriginalCommand(char * alias)
{
  for(int i=0;i<ALIASCOUNT;i++)
  {
    if(strcmp(alias,aliasHistory[i].aliascommand)==0)
    {
      return aliasHistory[i].originalcommand;
    }
  }
  return NULL;
}

void printRecentCommands()
{

    struct tm *tm,*tm2;
    int i=0;
    int temp=currentLogIndex-1;
    int temp2=temp;
    FILE * history;
    fclose(fopen("history.txt", "w"));
    while(i<RECENTCOMMANDS)
    {
        if(logHistory[temp].bit==1)
        {
            tm = localtime(&(logHistory[temp].timestamp));
            printf("%d. %-12s\t %04d-%02d-%02d %02d:%02d:%02d\t %d\n",i+1,logHistory[temp].command,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec,logHistory[temp].processId);
            history=fopen("history.txt","a");
            fprintf(history,"%d. %s\t %04d-%02d-%02d %02d:%02d:%02d\t %d\n",i+1,logHistory[temp].command,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec,logHistory[temp].processId);
            fflush(history);
            fclose(history);
            //printf("wrote into file!\n");
            temp--;
            if(temp<0)
                temp=24;
            i++;
            if(i==25){
            	break;
            }
        }
        else{
            break;
        }
    }
}

int execChild(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  addLogRecord(args[0],time(0),pid);
  if (pid == 0)
  {
    // Child process
  	int i=0;
  	while(args[i]!=NULL)
  	{
  		if(strcmp(args[i],"<")==0)
  		{
  			int fd=open(args[i+1],O_RDONLY);
  			//printf("%d\n",fd);
  			//printf("%d\n",fd);
  			if(fd<0)
  			{
  				printf("Input file does not exist\n");
  				exit(0);
  			}
  			else
  			{
  				dup2(fd,STDIN_FILENO);
  				args[i]=NULL;
  				args[i+1]=NULL;
  				i+=2;
  				continue;
  			}
  		}
  		if(strcmp(args[i],">")==0)
  		{
  			int fd=open(args[i+1],O_WRONLY);
  			if(fd<0)
  			{
  				printf("Output file does not exist\n");
  				exit(0);
  			}
  			else
  			{
  				dup2(fd,STDOUT_FILENO);
  				args[i]=NULL;
  				args[i+1]=NULL;
  				i+=2;
  				continue;
  			}
  		}
  		i+=1;
  	}
    if (execvp(args[0], args) == -1)
    {
      perror("exec error");
      return 1;
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("forking error");
    return 1;
  }
  else
  {
    // Parent process
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 0;
}

int execCommands(char **args)
{
  if(strcmp(args[0],"alias")==0)
  {
    logalias(args[1],args[2]);
    addLogRecord(args[0],time(0),getpid());
    return 0;
  }

  else if(strcmp(args[0],"log")==0)
  {
    printRecentCommands();
    addLogRecord(args[0],time(0),getpid());
    return 0;
  }

  else if(strcmp(args[0],"cd")==0)
  {
    addLogRecord(args[0],time(0),getpid());
    if (args[1] == NULL)
    {
      fprintf(stderr, "lsh: expected argument to \"cd\"\n");
      return 1;
    }
    else
    {
      if (chdir(args[1]) != 0)
      {
        perror("change directory error");
        return 1;
      }
    }
  }

  else if(strcmp(args[0],"sgown") == 0)
  {
    sgown(args[1],args[2]);
    return 0;
  }

  else if(strcmp(args[0],"sedit")==0)
  {
    pid_t pid, wpid;
    int status;

    pid = fork();
    addLogRecord(args[0],time(0),pid);
    if (pid == 0)
    {
      // Child process
      if (execvp("./sedit", args) == -1)
      {
        perror("couldnt start editor!");
        return 1;
      }
      exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
      // Error forking
      perror("couldnt fork!");
      return 1;
    }
    else
    {
      // Parent process
      do
      {
        wpid = waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 0;
  }

}

int executor(char **args)
{
  char * original = getOriginalCommand(args[0]);
  if(original!=NULL)
  {
    args[0] = original;
  }
  if( strcmp(args[0] , "sgown") == 0 || strcmp(args[0] , "alias") == 0 || strcmp(args[0] , "log") == 0 || strcmp(args[0] , "sedit") == 0 || strcmp(args[0],"cd") == 0)
  {
    return execCommands(args);
  }
  else
  {
    return execChild(args);
  }
  return 0;
}

int main(void)
{
  init();
  char prompt[3]="$$\0";
  char *parsedArgs[100];
  char* parsedArgsPiped[100];
  char input[50];
  while(1)
  {
      printf("%s ",prompt);
      gets(input);
      char* strpiped[2];
      if(parsePipe(input,strpiped)){
      	parseSpace(strpiped[0],parsedArgs);
      	parseSpace(strpiped[1],parsedArgsPiped);
      	execArgsPiped(parsedArgs,parsedArgsPiped);
	sleep(1);	
	continue;
      }
      if(strcmp(input,"quit")==0)
      {
          kill(0,SIGTERM);
      }
      if(strcmp(input,"ps -z")==0)
      {
      	system("ps -ef > psfile");
		    system("grep defunct psfile");
		    continue;
      }
      if(strcmp(input,"ls -z")==0)
      {
      	//printf("In ls -z");
      	char cwd[30];
      	if(getcwd(&cwd,sizeof(cwd))<0)
      		printf("Directory error\n");
      	else
      	{
      		DIR *directory=opendir(cwd);
      		struct dirent *dir;
      		dir=readdir(directory);
      		struct stat statbuf;
      		while(dir!=NULL)
      		{
      			stat(dir->d_name,&statbuf);
      			if(statbuf.st_size==0 && S_ISREG(statbuf.st_mode))
      			{
      				printf("%s \n",dir->d_name);
      			}
      			dir=readdir(directory);
      		} 
      	}
      	continue;
      	}
      char **tokens = split(input);
      tokens=executeWildCard(tokens);
      if(executor(tokens))
      {
          printf("cant execute command!\n");
      }
      free(tokens);

  }


  return 0;
}
