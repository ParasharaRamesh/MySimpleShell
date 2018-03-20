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

#define RECENTCOMMANDS 25
#define ALIASCOUNT  100

typedef struct commandentry{
    char command[30];
    int bit; // Set to 1 if this entry is used
}commandentry;

typedef struct alias{
  char aliascommand[10];
  char originalcommand[10];
  //struct alias *next;
}alias;

int aliasno=0;
alias aliasHistory[ALIASCOUNT];
commandentry recentcommands[RECENTCOMMANDS];

int currentindex=0;

void init(void)
{
  for(int i=0;i<RECENTCOMMANDS;i++)
  {
      recentcommands[i].bit=0;
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

int logCommand(char *command)
{
    if(currentindex>24)
    {
        currentindex=0;
    }
    memset(recentcommands[currentindex].command,0,sizeof(recentcommands[currentindex].command));
    strcpy(recentcommands[currentindex].command,command);
    recentcommands[currentindex].bit=1;
    currentindex+=1;
    return 1;
}

void printRecentCommands()
{
    int i=0;
    int temp=currentindex-1;
    /*for(int i=0;i<25;i++)
    {
        printf("%s\n",recentcommands[i].command);
    }*/
    while(i<RECENTCOMMANDS)
    {
        if(recentcommands[temp].bit==1)
        {
            printf("%d. %s\n",i+1,recentcommands[temp].command);
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
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
      return 1;
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
    return 1;
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 0;
}

int execCommands(char **args)
{
  if(strcmp(args[0],"log")==0)
  {
    printRecentCommands();
    return 0;
  }
  else if(strcmp(args[0],"cd")==0)
  {
    if (args[1] == NULL)
    {
      fprintf(stderr, "lsh: expected argument to \"cd\"\n");
      return 1;
    }
    else
    {
      if (chdir(args[1]) != 0)
      {
        perror("lsh");
        return 1;
      }
    }
    return 0;
  }
  else if(strcmp(args[0],"alias")==0)
  {
    logalias(args[1],args[2]);
    return 1;
  }
}

int executor(char **args)
{
  //
  char * original=getOriginalCommand(args[0]);
  if(original!=NULL)
  {
    strcpy(args[0],original);//replace it
  }
  if(execCommands(args)==1)
  {
    return 1;
  }
  if(execChild(args)==1)
  {
    return 1;
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
          break;
      }

      logCommand(input);
      char **tokens = split(input);
      if(executor(tokens))
      {
          printf("cant execute command!\n");
      }

  }

// printRecentCommands();

  return 0;
}
