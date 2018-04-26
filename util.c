#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include "util.h"
char ** split(char * input)
{
	char **result=(char **)malloc(sizeof(char *)*50);
	char *token = strtok(input," ");
	int i=0;
	while (token != NULL)
	{
		result[i]=token;
		i++;
		token = strtok(NULL, " ");
	}
	result[i]=NULL;
	return result;
}

int isWildCard(char * text)
{
	if(text==NULL)
	{
		return 0;
	}
	for(int i=0;i<strlen(text);i++)
	{
		if(*(text+i)=='*'||*(text+i)=='?')
		{
			return 1;
		}
	}
	return 0;
}

char ** executeWildCard(char ** tokens)
{
	//to do
	/*
		//store the items of the currentdirectory using the getAllItemsFromCurrDir()
		currdiritems=getAllItemsFromCurrDir()
		for every token with wildcard:
			regex=token
			matches=getAllMatches(regex,currdiritems)
			replace that token with all the matches ie we might have to shift right of the original tokens
			free(regex)
		//ie essentially if there is a command like ls a*
		//convert it into ls a1.txt a2.txt a3.txt by using this function
		//if ls a* | ls b* (just saying not necessary its valid)
		//convert it into ls a1.txt a2.txt | ls b1.txt b2.txt
		//i have given enough space for the tokens right after we split (about 50) so..
		//you can shift everything just like that (or have to think of other dynamic approach)
	*/
	//could be wrong didnt run it yet
	char ** ls =getAllItemsFromCurrDir();
	//char **result;
	int i=0;
	while(tokens[i]!=NULL)
	{
		char * regex;
		char ** matches;
		if(isWildCard(tokens[i]))
		{
			regex =tokens[i];
			//printf("regex : %s\n",regex);
			matches=getAllMatches(ls,regex);
			int matchescount=getCount(matches);
			int tokcount=getCount(tokens);
			int k,j;
			//printf("matches count : %d\t tokcount : %d",matchescount,tokcount);
			//shift everything to the right and leave matches count amount of space and then put this there
			for( k = i, j = 0; k < tokcount - 1 + matchescount; k++,j++)
			{
				//printf("\nk : %d", k);
				tokens[k]=matches[j];
				//printf("token %s,matches %s\n",tokens[k],matches[j]);
			}
			/*for(int k=0;k<matchescount;k++)
			{
				tokens[i+k]=matches[k];
			} */
		}
		i++;
	}
	//printf("\n tokens :");
	//printcontents(tokens);
	return tokens;
}

int getCount(char ** ls)
{
	int i=0;
	while(ls[i]!=NULL)
	{
		i++;
	}
	return i;
}

int wildcardcmp(char *pattern,char *text)//returns 1 if pattern matches 0 on failure
{//works for all cases with any regex with ? or *
	int m,n;
	if(pattern==NULL && text==NULL)
	{
		return 1;//empty pattern and text is true
	}
	else if(pattern==NULL)
	{
		m=strlen(text);
		n=0;
	}
	else if(text==NULL)
	{
		m=0;
		n=strlen(pattern);
	}
	else
	{
		m=strlen(text);
		n=strlen(pattern);
	}

	int T[m+1][n+1];//+1 cause null strings are a possibility
	memset(T, 0, sizeof(T[0][0]) * (m+1) * (n+1));//initialise 2d array with 0's
	//base conditions

		T[0][0]=1;
		//if(n==0)//if pattern is null then any text with this is false

		for(int i=1;i<m+1;i++)
		{
			T[i][0]=0;
		}

		//if text is null and if the jth character in the pattern is a * then look at the other j-1 characters as * covers empty string

		for(int j=1;j<n+1;j++)
		{
			if(*(pattern+j-1)=='*')
			{
				T[0][j]=T[0][j-1];
			}
		}

	//bottom up filling of the table
		for(int i=1;i<m+1;i++)
		{
			for(int j=1;j<n+1;j++)
			{
				if(*(text+i-1)==*(pattern+j-1)||*(pattern+j-1)=='?')
				{
					T[i][j]=T[i-1][j-1];
				}
				else if(*(pattern+j-1)=='*')
				{
					T[i][j]=T[i-1][j]||T[i][j-1];//first case is when * means include second case is when * means exclude
				}
				else//false
				{
					T[i][j]=0;
				}
			}
		}
	return T[m][n];//return value at the last cell
}

char ** getAllItemsFromCurrDir()//gets all the items in the curr directory and stores each item as a string
{
	DIR *d;
	char **result=(char **)malloc(sizeof(char *)*50);//have to free this somewhere ut not able to
    struct dirent *dir;
    d = opendir(getenv("PWD"));
    if (d)
		{
			int i=0;
			while ((dir = readdir(d)) != NULL)
			{
		  		result[i]=dir->d_name;
				i++;
		}
		//result[i+1]=NULL;
		result[i] = NULL;
		closedir(d);
    }
	return result;
}

void printcontents(char ** ls)//can use this utility function to print the content of a char ** array
{
	int k=0;
	while(ls[k]!=NULL)
	{
		printf("-%s-\n",ls[k]);
		k++;
	}
}

char ** getAllMatches(char ** ls,char * regex)//uses the regex and gets all the matches only as a char **
{
	int i=0,j=0;
	char **result=(char **)malloc(sizeof(char *)*50);//have to free this too but not able to
	while(ls[i]!=NULL)
	{
		//if(strcmp(ls[i],"")!=0&&strcmp(ls[i],".")!=0&&strcmp(ls[i],"..")!=0)
		//{
			if(wildcardcmp(regex,ls[i]))
			{
				//add to the list
				result[j]=ls[i];
				j++;
			}
		//}
		i++;
	}
	//printf("all matches\n");
	result[j]=NULL;
	//printcontents(result);
	return result;
}

int isCommandHavingWildcard(char ** command)
{
 	int i=0;
 	while(command[i]!=NULL)
 	{
 		if(isWildCard(command[i]))
 		{
 			return 1;
 		}
 		i++;
 	}
 	return 0;
}

int getOccurenceCount(char * path,char * word)
 {
 	FILE *fptr;
  /* Try to open file */
     fptr = fopen(path, "r");
 	 /* Exit if file not opened successfully */
     if (fptr == NULL)
     {
         //printf("\t\t\tUnable to open file.\n");
         return -1;
     }
 	int BUFFER_SIZE=2000;
  	char str[BUFFER_SIZE];
     char *pos;
 
     int index, count;
 
     count = 0;
 
     // Read line from file till end of file.
     while ((fgets(str, BUFFER_SIZE, fptr)) != NULL)
     {
         index = 0;
 
         // Find next occurrence of word in str
         while ((pos = strstr(str + index, word)) != NULL)
         {
             // Index of word in str is
             // Memory address of pos - memory
             // address of str.
             index = (pos - str) + 1;
 
             count++;
         }
     }
     return count;
 }
 
 void sgown(const char *name, char * searchstring)
 {
     DIR *dir;
     struct dirent *entry;
     if (!(dir = opendir(name)))
 		{
 			//printf("failed!\n");
       return;
 		}
     while ((entry = readdir(dir)) != NULL)
 		{
 				char path[1024];
         if (entry->d_type == DT_DIR)
 				{
             if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                 continue;
             snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
             sgown(path,searchstring);
         }
 		 		else
 				{
 				char fullpath[1024];
 					strcpy(fullpath,name);
 					strcat(fullpath,"/");
 					strcat(fullpath,entry->d_name);
 					int count=getOccurenceCount(fullpath,searchstring);
 					if(count!=0 && count!=-1)
 					{
 					printf("file: %s count: %d\n",fullpath,count);
 					}
 		  	}
   	}
     closedir(dir);
 }