#include<stdlib.h>
#include<stdio.h>
#include<string.h>
char ** split(char * input)
{
	//initialize 2d array
  //I think the maximum number of arguments we can get is 6
	char **result=(char **)malloc(sizeof(char *)*6);
	char *token = strtok(input," ");
	int i=0;
//  printf("aaa\n");
	while (token != NULL)
	{
		//printf("%s\t%d\n",token,i);
		//append into 2d array somehow!
		result[i]=token;
		//printf("\t%s\n",result[i]);
		i++;
		token = strtok(NULL, " ");
	}

		return result;
}
