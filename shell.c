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

    struct tm *tm;
    int i=0;
    int temp=currentLogIndex-1;
    while(i<RECENTCOMMANDS)
    {
        if(logHistory[temp].bit==1)
        {
            tm = localtime(&(logHistory[temp].timestamp));
            printf("%d. %s\t %04d-%02d-%02d %02d:%02d:%02d\t %d\n",i+1,logHistory[temp].command,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec,logHistory[temp].processId);
            FILE * history=fopen("history.txt","a");
            fprintf(history,"%s\t %04d-%02d-%02d %02d:%02d:%02d\t %d\n",logHistory[temp].command,tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,tm->tm_hour, tm->tm_min, tm->tm_sec,logHistory[temp].processId);
            fflush(history);
            fclose(history);
            printf("wrote into file!\n");
            temp--;
            if(temp<0)
                temp=24;
            i++;
            if(i==25)
                break;
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
    memset(args[0],0,sizeof(args[0]));
    strcpy(args[0],original); //replace it
  }
  if( strcmp(args[0] , "alias") == 0 || strcmp(args[0] , "log") == 0 || strcmp(args[0] , "sedit") == 0 || strcmp(args[0],"cd") == 0)
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
  char input[50];
  while(1)
  {
      printf("%s ",prompt);
      gets(input);

      if(strcmp(input,"quit")==0)
      {
          exit(0);
      }

      char **tokens = split(input);
      if(executor(tokens))
      {
          printf("cant execute command!\n");
      }

  }


  return 0;
}
