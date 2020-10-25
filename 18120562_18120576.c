#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//Xoa khoang trang dau chuoi
void delSpace(char* str)
{
	while(str[0] == ' ')
	{
		int i;
		for(i = 0; i < strlen(str); i++)
			str[i] = str[i+1];
	}
}

//Chia lenh nhap vao thanh tung phan
char** splitCommand(char* cmd, int* count)
{
	char** result;
	
	char* temp;	//Chuoi tam de luu cac phan cua lenh
	(*count) = 0; //Bien dem so luong phan cua lenh
	int indexResult = 0;

	/* Dem so luong phan cua lenh */
	int i;
	for (i = 0 ; i < strlen(cmd); i++)
	{
		if(cmd[i] == ' ' && cmd[i+1] != ' ' && cmd[i+1] != '\0')
			(*count)++;
	}
	(*count)++;
	
	result = (char**)malloc(((*count) + 1) * sizeof(char*));

	/* Tach lenh */
	temp = strtok(cmd, " \n");

	while(temp != NULL)
	{
		delSpace(temp);
		result[indexResult] = temp;
		indexResult++;
		temp = strtok(NULL, " \n");
	}
	result[indexResult] = NULL;
	return result;
}


//Return 3: lenh > hoac <
//Return 1: lenh thuong
int getTypeOfCommand (char** cmd, int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		if(strcmp(cmd[i], "<") == 0 || strcmp(cmd[i], ">") == 0)
			return 3;
	}
	return 1;
}


//Xu ly lenh thuong.
void handleType1Command(char** cmd, int count)
{
	
	pid_t new_pid;
	int child_status;
	
	new_pid = fork(); 
	switch (new_pid)
	{
		case -1: 
			printf( "Loi: Khong the tao tien trinh." );
			break;
		case 0: 
			execvp(cmd[0], cmd);			
		 	break;
		default: 
			wait( &child_status ); //Tien trinh cha doi tien trinh con ket thuc.
			break;
	}
}

//Xu ly lenh < hoac >
void handleType3Command(char** cmd, int count)
{
	pid_t new_pid;
	int child_status;
	
	char* filePath;
	int type = -1; // 0: < va 1: >
	int i;
	char* temp;
	for (i = 0; i < count; i++)
	{
		if(type == -1)
		{
			if(strcmp(cmd[i], "<") == 0)
			{
				type = 0;
				filePath = cmd[i+1];

			}

			if(strcmp(cmd[i], ">") == 0)
			{
				type = 1;
				filePath = cmd[i+1];
			}
		}
		else
			cmd[i - 1] = NULL;
	}
	cmd[i - 1] = NULL;

	new_pid = fork(); 
	switch (new_pid)
	{
		case -1: 
			printf( "Loi: Khong the tao tien trinh." );
			break;
		case 0: 
		{
			int fileDes;
			if(type == 0)
			{
				fileDes = open(filePath, O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
			}
			else 
				fileDes = open(filePath, O_WRONLY | O_TRUNC | O_CREAT,  S_IWUSR | S_IWGRP |S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
			
			dup2(fileDes, type);
			close(fileDes);
			
			execvp(cmd[0], cmd);
		}			
		 	break;
		default: 
			wait( &child_status ); //Tien trinh cha doi tien trinh con ket thuc.
			break;
	}
	
}

// Them lenh vao history
void addCmdToHistory(char*** cmdHistory, int* count, char* newCmd)
{		
	if(*count == 0)
		*cmdHistory = (char**) malloc(sizeof(char*) * 1);	
	else
		*cmdHistory = (char**) realloc(*cmdHistory, sizeof(char*) * (*count + 1));

	*count = *count + 1;

	(*cmdHistory)[*count - 1] = malloc(sizeof(char) * strlen(newCmd));

	strcpy((*cmdHistory)[*count - 1], newCmd);
}

// Hien thi cac lenh da dung
void showHistory(char** cmdHistory, int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		printf("%5d  ", i + 1);
		fputs(cmdHistory[i], stdout);
	}
}

// Lay ra lenh da dung gan nhat
char* getTheLastCmd(char** cmdHistory, int count)
{
	if(count == 0)
		return NULL;

	return cmdHistory[count - 1];
}

// Kiem tra lenh co phai la !! khong
int isCmdExecuteLastCmd(char* cmd)
{
	int i;
	int len;
	len = strlen(cmd);
	for(i = 0; i < len - 1; i++)
	{
		if(cmd[i] == '!' && cmd[i + 1] == '!')
			return 1;
	} 

	return 0;
}

void main()
{
	char** commandHistory = NULL;
	int countHistory = 0;

	while(1)
	{
		char* command;
		size_t size = 0;
		int count;
		printf("> ");
		getline(&command, &size, stdin);
		
		// Neu lenh khong phai !!, them vao history
		if(isCmdExecuteLastCmd(command) == 0)
			addCmdToHistory(&commandHistory, &countHistory, command);		

		char **cmd;
		cmd = splitCommand(command, &count);
		
		// Neu lenh la !!
		if(strcmp(cmd[0], "!!") == 0 && count == 1)
		{
			char* lastCmd = getTheLastCmd(commandHistory, countHistory);
			
			// Neu history rong
			if(lastCmd == NULL)
			{
				printf("No command in history\n");
				continue;
			}
			
			command = malloc(sizeof(char) * strlen(lastCmd));
			strcpy(command, lastCmd);			
			addCmdToHistory(&commandHistory, &countHistory, command);
			cmd = splitCommand(command, &count);
		}	

		if(strcmp(cmd[0], "history") == 0 && count == 1)
		{
			showHistory(commandHistory, countHistory);
			continue;
		}	
		
		if(strcmp(cmd[0], "cd") == 0 && count == 2)
		{	
			chdir(cmd[1]);
			continue;
		}
		
		if(strcmp(cmd[0], "exit") == 0 && count == 1)
			return;
		if(getTypeOfCommand(cmd, count) == 1)
			handleType1Command(cmd, count);
		else
		{
			if (getTypeOfCommand(cmd, count) == 3)
				handleType3Command(cmd, count);
		}
		free(command);
		free(cmd);
	}
}

	
	