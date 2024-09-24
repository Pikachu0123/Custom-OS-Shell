#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // For open(), O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC
#include <sys/types.h>
#include <sys/stat.h>
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
	char **command;
	int status;

	int copy_stdout = dup(STDOUT_FILENO);
	int copy_stdin = dup(STDIN_FILENO);
	
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
		
		int background_process = 0;
		int redirection = 0, input_1 = -1, input_2 = -1, input_3 = -1;
		for(int i=0; command[i] != NULL; i++){
			if (strcmp(command[i], "<") == 0 || strcmp(command[i], ">") == 0 || strcmp(command[i], ">>") == 0){
				redirection = 1;
				if (strcmp(command[i], "<") == 0)
					input_1 = i;
				if (strcmp(command[i], ">") == 0)
					input_2 = i;
				if (strcmp(command[i], ">>") == 0)
					input_3 = i;
			} // check for redirection opertor

			if (strcmp(command[i], "&") == 0){
					background_process = 1;
			} // check if want process to run in background
		}

		pid_t pid = fork();
		int file_descriptor = -1;
		
		if (pid < 0){
			fprintf(stderr, "Error in making child process\n");
		}

		else if (pid == 0){
			// child process
			if (redirection == 1){
				// redirection operator is present

				if (input_1 >= 0){
					// '<' is present
					file_descriptor = open(command[input_1+1] , O_RDONLY, 0644);
					dup2(file_descriptor, STDIN_FILENO);
				}

				if (input_2 >= 0){
					// '>' is present and '>>' is NOT present
					// '>>' takes priority over '>'.
					// ./a.out > testfile.txt >> newtestfile.txt --> newtestfile.txt gets output appended but testfile.txt BECOMES EMPTY (even if it contained some content earlier)
					file_descriptor = open(command[input_2+1] , O_WRONLY | O_CREAT | O_TRUNC, 0644);
					// O_TRUNC reduces the size of file to 0 bytes. removes the previous content present in it
					dup2(file_descriptor, STDOUT_FILENO);
				}

				if (input_3 >= 0){
					// '>>' is present
					file_descriptor = open(command[input_3+1] , O_WRONLY | O_CREAT | O_APPEND, 0644);
					dup2(file_descriptor, STDOUT_FILENO);
				}

				int start = 7;
				if (input_1 >= 0 and input_1 < start) start = input_1;
				if (input_2 >= 0 and input_2 < start) start = input_2;
				if (input_3 >= 0 and input_3 < start) start = input_3;

				for(int i=start; command[i] != NULL; i++)
					command[i] = NULL;
				// we configure the array (command[i] = NULL) after checking all three
				// redirection operators, because there may be several present in one command
				// like ./a.out > file1 >> file2
				// or ./a.out < file1 >> file2
				// or ./a.out < file1 > file2 >> file3
			}

			execvp(command[0], command);
			fprintf(stderr, "Error in running command %s\n", command[0]);
			// this statement will only run when execvp is unsuccessful in running
		}

		else{
			// parent process
			if (background_process == 0){
				waitpid(pid, &status, WUNTRACED);
				// we dont want the process to run in background, so we want the child process to finish first
			}
			else{
				// we want process to run in background so we can continue using the shell, so we dont wait for the child process to finish first
				// we print the process ID
				printf("%d\n", pid);
			}
			if (redirection == 1){
				// restore stdout and stdin
				dup2(copy_stdin, STDOUT_FILENO);
				// dup2(a, b) means value of 'a' given to 'b', not vice versa
				dup2(copy_stdout, STDIN_FILENO);
				close(copy_stdout);
				close(copy_stdin);
				close(file_descriptor);
			}
		}

		// printf("nah");
		free(command);
	}

	free(input);

	return 0;
}
