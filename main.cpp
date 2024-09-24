#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

void removeExtraSpace(char *input){
	char *clean_string = (char *)malloc(sizeof(char) * (strlen(input) + 1)); // sizeof(char) == 1
	// + 1 is for null terminator

	if (clean_string == NULL){
		fprintf(stderr, "Memory Allocation for string cleaning failed!\n");
		return;
	}

	int space_taken = 0;
	int j = 0; // pointer for the clean string
	for(int i=0; i<strlen(input); i++){
		if (input[i] == ' '){
			space_taken = 1;
		}

		else{
			if (space_taken == 1){
				clean_string[j] = ' ';
				j++;
				space_taken = 0;
			}
			clean_string[j] = tolower(input[i]);
			j++;
		}
	}

	clean_string[j] = '\0'; // null termintor
	
	strcpy(input, clean_string);
	free(clean_string);
}

char **breakInputString(char *input){
	char **command = (char **)malloc(sizeof(char *) * 8);
	// Assuming 7 command line arguments with last argument as NULL

	const char *delimiter = " ";
	char *token = strtok(input, delimiter);
	int index = 0;

	while(token != NULL){
		command[index] = token;
		index++;
		token = strtok(NULL, delimiter);
	}

	command[index] = NULL;
	return command;
}

char *stripQuotes(char *path){
	int len = strlen(path);
	char *new_path;
	if (len >= 2 and ((path[0] == '"' and path[len-1] == '"') or (path[0] == '\'' and path[len-1] == '\''))){
		new_path = (char *)malloc(sizeof(char) * (len - 1));
		for(int i=1; i<len-1; i++){
			new_path[i-1] = path[i];
		}
		new_path[len-1] = '\0'; // adding null 
	}

	else{
		new_path = path;
	}

	return new_path;
}

void changeDirectory(char **command){
	// printf("command[1]: %s\n", command[1]);
	// command[1] = stripQuotes(command[1]);
	
	char *dir = command[1];	
	// if (dir[0] == '\'' or dir[0] == '"'){
	// 	for(int i=2; i < 8 && command[i] != NULL; i++){
	// 		strcat(dir, command[i] + ' ');
	// 		int len = strlen(command[i]);
	// 		if (command[i][len-1] == '\'' or command[i][len-1] == '"'){
	// 			break;
	// 		}
	// 	}
	// 	dir[strlen(dir)-1] = '\0'; // end of line
	// }

	if (dir == NULL or chdir(dir) < 0){
		fprintf(stderr, "Error in changing directory to %s\n", dir);
	}
	return;
}

int main(){

	char *input;
	size_t input_size = 0;
	char **command;
	int status;
	while(1){
		input = readline("unixsh> ");
		// using getline doesnt allow for arrow movement (left and right) 
		// and also history (up and down)
		
		if (input == NULL){
			break;
			// EOF
		}

		if (strlen(input) == 0){
			free(input);
			continue;
			// empty command
		}
	
		add_history(input);		
		// input[strlen(input) - 1] = '\0'; // getline includes the newline (\n) with it
		removeExtraSpace(input);
		if (strcmp(input, "exit") == 0){
			break;
		}
		
		command = breakInputString(input);

		if (strcmp(command[0], "cd") == 0){
			changeDirectory(command);
			continue;
		}

		pid_t pid = fork();
		
		if (pid < 0){
			fprintf(stderr, "Error in making child process\n");
		}

		else if (pid == 0){
			// child process
			execvp(command[0], command);
			fprintf(stderr, "Error in running command %s\n", command[0]);
			// this statement will only run when execvp is unsuccessful in running
		}

		else{
			// parent process
			waitpid(pid, &status, WUNTRACED);
		}

		// printf("nah");
		free(command);
	}

	free(input);

	return 0;
}