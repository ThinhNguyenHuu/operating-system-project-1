#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1
#define READ_END 0
#define WRITE_END 1

//Xoa khoang trang dau chuoi, cuoi chuoi, khoang trang thua giua chuoi
void delSpace(char* str)
{
	/* Xoa khoang trang dau chuoi */
	while(str[0] == ' ')
	{
		int i;
		for(i = 0; i < strlen(str) - 1; i++)
			str[i] = str[i+1];
		str[i] = '\0';
	}

	/* Xoa khoang trang cuoi chuoi */
	while(str[strlen(str) - 1] == ' ' || str[strlen(str) -1] == '\n')
	{
		str[strlen(str) - 1] = '\0';
	}

	/* Xoa khoang trang giua chuoi */
	int i;
	for(i = 0; i < strlen(str); i++)
	{
		/* Khong xoa khoang trang trong cap dau " " */
		if(str[i] == '\"')
		{
			i++;
			while(str[i] != '\"' && str[i] != '\0') 
				i++;
			continue;
		}

		if(str[i] == ' ' && (str[i + 1] == ' ' || str[i+1] == '\0'))
		{
			int j;
			for(j = i; j < strlen(str) - 1; j++)
				str[j] = str[j + 1];
			str[j] = '\0';
			i--;
		}
	}

}

//Chia lenh nhap vao thanh tung phan
char** splitCommand(char* cmd, int* count)
{
	char** result;

	char temp[100];	//Chuoi tam de luu cac phan cua lenh
	(*count) = 0;   //Bien dem so luong phan cua lenh
	int indexResult = 0;

	/* Dem so luong phan cua lenh */
	int i;
	for (i = 0 ; i < strlen(cmd); i++)
	{
		if(cmd[i] == '\"')
		{
			i++;
			while(cmd[i] != '\"' && cmd[i] != '\0') 
				i++;
			continue;
		}

		if(cmd[i] == ' ' && cmd[i+1] != ' ' && cmd[i+1] != '\0')

			(*count)++;
	}
	(*count)++;

	result = (char**)malloc(((*count) + 1) * sizeof(char*));

	/* Tach lenh */
	int indexTemp = 0;
	for(i = 0; i < strlen(cmd); i++)
	{
		if(cmd[i] == '\"')
		{
			i++;
			while(cmd[i] != '\"' && cmd[i] != '\0') 
			{
				temp[indexTemp] = cmd[i];
				indexTemp++;
				i++;
			}
			continue;
		}

		if(cmd[i] != ' ')
		{
			temp[indexTemp] = cmd[i];
			indexTemp++;
		}
		else
		{
			temp[indexTemp] = '\0';
			result[indexResult] = (char*)malloc((strlen(temp) + 1));
			int j;
			for(j = 0; j < strlen(temp); j++)
				result[indexResult][j] = temp[j];
			result[indexResult][j] = '\0';

			indexResult++;
			indexTemp = 0;
		}
	}
	temp[indexTemp] = '\0';
	result[indexResult] = (char*)malloc((strlen(temp) + 1));
	int j;
	for(j = 0; j < strlen(temp); j++)
		result[indexResult][j] = temp[j];
	result[indexResult][j] = '\0';

	indexResult++;

	result[indexResult] = NULL;
	return result;
}

//Ham phan loai lenh
//Return 3: lenh > hoac <
//Return 1: lenh thuong
//Return 4: lenh dung pipe
int getTypeOfCommand (char** cmd, int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		if(strcmp(cmd[i], "|") == 0)
			return 4;

		if(strcmp(cmd[i], "<") == 0 || strcmp(cmd[i], ">") == 0 || strcmp(cmd[i], ">>") == 0)
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
			printf( "Error: Cannot create process.\n" );
			break;
		case 0: 
			execvp(cmd[0], cmd);
			exit(EXIT_SUCCESS);			
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

	char* filePathInput = NULL;
	char* filePathOutput = NULL;
	char* filePathOutputAppend = NULL;

	int flag = 0;
	int count2 = 0;

	int orderOut = 0;
	int orderOutAppend = 0;
	int i;
	for (i = 0; i < count; i++)
	{
		if(strcmp(cmd[i], "<") == 0)
		{
			filePathInput = cmd[i+1];
			flag = 1;
		}

		if(strcmp(cmd[i], ">") == 0)
		{
			filePathOutput = cmd[i+1];
			flag = 1;
			orderOut = i;
		}

		if(strcmp(cmd[i], ">>") == 0)
		{
			filePathOutputAppend = cmd[i+1];
			flag = 1;
			orderOutAppend = i;
		}

		if(flag == 0)
			count2++;
	}


	char** newCmd;
	newCmd = (char**)malloc((count2 + 1) * sizeof(char*));
	
	for(i = 0; i < count2; i++)
	{
		newCmd[i] = (char*)malloc(strlen(cmd[i]) + 1);
		int j;
		for(j = 0; j < strlen(cmd[i]); j++)
			newCmd[i][j] = cmd[i][j];
		newCmd[i][j] = '\0';
	}
	newCmd[i] = NULL;

	new_pid = fork(); 
	switch (new_pid)
	{
		case -1: 
			printf( "Error: Cannot create process.\n" );
			break;
		case 0: 
		{
			int fileDesInput = 0;
			int fileDesOutput = 1;
			int fileDesOutputAppend = 1;
			if(filePathInput != NULL || filePathOutput != NULL || filePathOutputAppend != NULL)
			{
				if(filePathInput != NULL)
				{
					fileDesInput = open(filePathInput, O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
					if(fileDesInput < 0)
					{
						printf("Error: Cannot open file.\n");
						exit(EXIT_FAILURE);
					}
					else
					{
						dup2(fileDesInput, STDIN);
						close(fileDesInput);
					}
				}
				
				if(filePathOutput != NULL && filePathOutputAppend != NULL)
				{
					if(orderOut > orderOutAppend)
					{
						fileDesOutput = open(filePathOutput, O_WRONLY | O_TRUNC | O_CREAT,  S_IWUSR | S_IWGRP |S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
						if(fileDesOutput < 0)
						{
							printf("Error: Cannot open file.\n");
							exit(EXIT_FAILURE);
						}
						else
						{
							dup2(fileDesOutput, STDOUT);
							close(fileDesInput);
						}
					}
					else
					{
						fileDesOutputAppend = open(filePathOutputAppend, O_WRONLY | O_APPEND | O_CREAT,  S_IWUSR | S_IWGRP |S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
						if(fileDesOutputAppend < 0)
						{
							printf("Error: Cannot open file.\n");
							exit(EXIT_FAILURE);
						}
						else
						{
							dup2(fileDesOutputAppend, STDOUT);
							close(fileDesInput);
						}
					}
				}
				else
				{
					if(filePathOutput != NULL)
					{
						fileDesOutput = open(filePathOutput, O_WRONLY | O_TRUNC | O_CREAT,  S_IWUSR | S_IWGRP |S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
						if(fileDesOutput < 0)
						{
							printf("Error: Cannot open file.\n");
							exit(EXIT_FAILURE);
						}
						else
						{
							dup2(fileDesOutput, STDOUT);
							close(fileDesInput);
						}
					}

					if(filePathOutputAppend != NULL)
					{
						fileDesOutputAppend = open(filePathOutputAppend, O_WRONLY | O_APPEND | O_CREAT,  S_IWUSR | S_IWGRP |S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
						if(fileDesOutputAppend < 0)
						{
							printf("Error: Cannot open file.\n");
							exit(EXIT_FAILURE);
						}
						else
						{
							dup2(fileDesOutputAppend, STDOUT);
							close(fileDesInput);
						}
					}
				}
				execvp(newCmd[0], newCmd);
				exit(EXIT_SUCCESS);

			}
			else
			{
				printf("Error: Don't have filename in command.\n");
				exit(EXIT_FAILURE);
			}
		}			

		 	break;
		default: 
			wait( &child_status ); //Tien trinh cha doi tien trinh con ket thuc.
			break;
	}
	for(i = 0; i < count2; i++)
		if(newCmd[i] != NULL)
			free(newCmd[i]);
	if(newCmd != NULL)
		free(newCmd);
}

// Them lenh vao history
void addCmdToHistory(char*** cmdHistory, int* count, char* newCmd)
{		
	if(*count == 0)
		*cmdHistory = (char**) malloc(sizeof(char*) * 1);	
	else
		*cmdHistory = (char**) realloc(*cmdHistory, sizeof(char*) * (*count + 1));

	*count = *count + 1;

  (*cmdHistory)[*count - 1] = strdup(newCmd);
}

// Hien thi cac lenh da dung
void showHistory(char** cmdHistory, int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		printf("%5d  ", i + 1);

		fputs(cmdHistory[i], stdout);
		printf("\n");
	}
}

// Lay ra lenh da dung gan nhat
char* getTheLastCmd(char** cmdHistory, int count)
{
	if(count == 0)
		return NULL;

	return cmdHistory[count - 1];
}

// Lay ra lenh o vi tri pos
char* getCmdAt(char** cmdHistory, int count, int pos)
{
	if(count < pos)
		return NULL;

	return cmdHistory[pos - 1];
}


// Return 1: 2 chuoi bang nhau
// Return 0: 2 chuoi khong bang 
int isStringEqual(char* str1, char* str2)
{
	if(str1 == NULL || str2 == NULL)
		return 0;

	int len1, len2;
	len1 = strlen(str1);
	len2 = strlen(str2);
	if(len1 != len2)
		return 0;

	int i;
	for(i = 0; i < len1; i++)
		if(str1[i] != str2[i])
			return 0;

	return 1;
}


// Tach lenh dung pipe
void splitCommandUsePipe(char** cmd, int count, char*** firstCmd, char*** secondCmd)
{
	int i, temp = -1;
	for(i = 0; i < count; i++)
		if(strcmp(cmd[i], "|") == 0)
			temp = i;

	if(temp < 0)
		return;

  // Lay ra lenh thu nhat
	*firstCmd = (char**) malloc(sizeof(char*) * temp);
	for(i = 0; i < temp; i++)
    (*firstCmd)[i] = strdup(cmd[i]);

  // Lay ra lenh thu hai
	*secondCmd = (char**) malloc(sizeof(char*) * (count - temp - 1));
	for(i = temp + 1; i < count; i++)
    (*secondCmd)[i - temp - 1] = strdup(cmd[i]);
}



// Xu ly lenh dung pipe
void handleType4Command(char** cmd, int count)
{
	pid_t pid, cpid;
	int status, childStatus;
	int fd[2];
	char** firstCmd;
	char** secondCmd;
	splitCommandUsePipe(cmd, count, &firstCmd, &secondCmd);	

	
	pipe(fd);		// Tao pipe
	pid = fork();	// Tao process
	switch(pid)
	{
		case -1:
			printf( "Error: Cannot create process.\n" );
			break;

		case 0:		// Thuc thi lenh dau tien va ghi len pipe
			dup2(fd[WRITE_END], STDOUT);	
			close(fd[READ_END]);	    	
    		close(fd[WRITE_END]);			

			execvp(firstCmd[0], firstCmd);
			_exit(EXIT_SUCCESS);
			break;

		default:
			cpid = fork();	// Tao process
			switch(cpid)
			{
				case -1:
					printf( "Error: Cannot create process.\n" );
					break;
	
				case 0:		// Doc ket qua cua lenh dau tien tu pipe va thuc thi lenh thu hai
					dup2(fd[READ_END], STDIN);	
					close(fd[READ_END]);	    	
					close(fd[WRITE_END]);			

					execvp(secondCmd[0], secondCmd);
					_exit(EXIT_SUCCESS);
					break;

				default:	// Doi cac process con ket thuc
					close(fd[READ_END]);	    	
					close(fd[WRITE_END]);
					wait(&status);
					wait(&childStatus);
					break;
			}
			break;	
	}
}

void main()
{
	char** commandHistory = NULL;
	int countHistory = 0;
	char* prePath = NULL;
	char* tempPath = NULL;
	char* pwd = getenv("PWD");	
	char* tempPath2 = NULL;

	tempPath = (char*)malloc(strlen(pwd) + 1);
	int i;
	for(i = 0; i < strlen(pwd); i++)
		tempPath[i] = pwd[i];
	tempPath[i] = '\0';

	while(1)
	{
		char* command;
		size_t size = 0;
		printf("osh> ");
		fflush(stdin);
		getline(&command, &size, stdin);

		delSpace(command);

		// Neu lenh rong
		if(strcmp(command, "") == 0)
			continue;

		
		char **cmd;
		int count;
		cmd = splitCommand(command, &count);

		// Neu lenh la !!
		if(strcmp(cmd[0], "!!") == 0 && count == 1)
		{
      // Lay ra lenh o vi tri cuoi cung
			char* lastCmd = getTheLastCmd(commandHistory, countHistory);
		
    	// Neu history rong
			if(lastCmd == NULL)
			{
				printf("No command in history.\n");
				continue;
			}

			command = strdup(lastCmd);	

			puts(command);
			cmd = splitCommand(command, &count);

		}	

		// Neu lenh la ! + 'number'
		if(count == 1 && cmd[0][0] == '!' && strlen(cmd[0]) > 1)
		{
		int number = 0, i;
		int flag = 0;

		// Lay ra 'number'
		for(i = 1; i < strlen(cmd[0]); i++)
		{
			int digit = cmd[0][i] - '0';
			if(digit < 0 || digit > 9)
			{
			flag = 1;
			break;
			}
			else
			{
			number = number * 10 + digit;
			}
		}

		// Neu 'number' khong phai so
		if(flag == 1)
			continue;

		// Lay ra lenh o vi tri 'number'
		char* selectedCmd = getCmdAt(commandHistory, countHistory, number);

		// Neu 'number' > history stack
		if(selectedCmd == NULL)
			continue;

		command = strdup(selectedCmd);

				puts(command);
				cmd = splitCommand(command, &count);
		}

		//Them lenh vao history
		char* lastCmd = getTheLastCmd(commandHistory, countHistory);
		if(isStringEqual(command, lastCmd) == 0)
			addCmdToHistory(&commandHistory, &countHistory, command);	


		if(strcmp(cmd[0], "history") == 0 && count == 1)
		{
			showHistory(commandHistory, countHistory);	
			continue;
		}	


		if(strcmp(cmd[0], "cd") == 0)
		{
			tempPath2 = (char*)malloc(strlen(tempPath) + 1);

			int i;
			for(i = 0; i < strlen(tempPath); i++)
				tempPath2[i] = tempPath[i];
			tempPath2[i] = '\0';


			if(cmd[1] == NULL || strcmp(cmd[1], "~") == 0)
			{
				char* home =  getenv("HOME");
				tempPath2 = (char*)malloc(strlen(home) + 1);

				for(i = 0; i < strlen(home); i++)
					tempPath2[i] = home[i];
				tempPath2[i] = '\0';

				chdir(home);
			}
			else if(strcmp(cmd[1], "..") == 0)
			{
				while(tempPath2[strlen(tempPath2) - 1] != '/' )
					tempPath2[strlen(tempPath2) - 1] = '\0';
				
				chdir(tempPath2);
			}
			else if(strcmp(cmd[1], "-") == 0)
			{
				if(prePath == NULL)
					printf("Don't have previous path.\n");
				else
				{
					tempPath2 = (char*)malloc(strlen(prePath) + 1);

					for(i = 0; i < strlen(prePath); i++)
						tempPath2[i] = prePath[i];
					tempPath2[i] = '\0';

					chdir(prePath);
				}
			}
			else
			{
				tempPath2 = (char*)malloc(strlen(cmd[1]) + 1);


				for(i = 0; i < strlen(cmd[1]); i++)
					tempPath2[i] = cmd[1][i];
				tempPath2[i] = '\0';

				chdir(cmd[1]);
			}
			
			prePath = (char*)malloc(strlen(tempPath) + 1);

			for(i = 0; i < strlen(tempPath); i++)
				prePath[i] = tempPath[i];
			prePath[i] = '\0';

			tempPath = (char*)malloc(strlen(tempPath2) + 1);

			for(i = 0; i < strlen(tempPath2); i++)
				tempPath[i] = tempPath2[i];
			tempPath[i] = '\0';
			
			continue;
		}

		if(strcmp(cmd[0], "exit") == 0)
		{
			int i;
			for(i = 0; i < countHistory; i++)
			{
				if(commandHistory[i] != NULL)
					free(commandHistory[i]);
			}
			if(commandHistory != NULL)
				free(commandHistory);
			if(prePath != NULL)
				free(prePath);
			if(tempPath != NULL)
				free(tempPath);
			if(tempPath2 != NULL)
				free(tempPath2);

			exit(EXIT_SUCCESS);
		}


		if(getTypeOfCommand(cmd, count) == 1)
			handleType1Command(cmd, count);
		else if (getTypeOfCommand(cmd, count) == 3)
				handleType3Command(cmd, count);
		else if (getTypeOfCommand(cmd, count) == 4)
				handleType4Command(cmd, count);


		
		if(command != NULL)
			free(command);
		int i;
		for(i = 0; i <= count; i++)
		{
			if(cmd[i] != NULL)
				free(cmd[i]);
		}

		if(cmd != NULL)
			free(cmd);

	}

}