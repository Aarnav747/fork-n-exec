#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int input(char in_buff[], size_t size_inbuff) {
	char *fgets_ret = fgets(in_buff, size_inbuff, stdin);
	if (fgets_ret == NULL) return -1;

	return 0;
}

int inbuilt_inst(char *args[], int case_no) {
	switch(case_no) {
		case 1: {
			if (args[1] == NULL) return -1;
			                                        		if (chdir(args[1]) == -1) return -1;										break;
		}			
	}
		                                                return 0;                               
}
			

void execute(char *args[], char *filename, int std_fd);

int tokenizing(char in_buff[], char *args[], size_t size_args) {
	args[0] = strtok(in_buff, " \n");

	if (strcmp(args[0], "exit") == 0) {
		return 1;
	}

	int i = 1;
	int out_redirect;
	int in_redirect;
	char *token = NULL;
	for (i = 1; i < size_args; i++) {
		token = strtok(NULL, " \n");

		if (token == NULL) break;

		args[i] = token;

		out_redirect = strcmp(token, ">");
		in_redirect = strcmp(token, "<");


		if (out_redirect == 0 || in_redirect == 0) {
			token = strtok(NULL, " \n");
			if (token == NULL) return -1;
			args[i+1] = NULL;
			args[i] = NULL;
			break;
		}
	}

	if (out_redirect == 0) {
		execute(args, token, STDOUT_FILENO);
		return 2;
	} else if (in_redirect == 0) {
		execute(args, token, STDIN_FILENO);
		return 2;
	}

	args[i] = NULL;

	if (strcmp(args[0], "cd") == 0) {
		inbuilt_inst(args, 1);
		return 2;
	}

	return 0;
}

void execute(char *args[], char* filename, int std_fd) {
	pid_t child = fork();

	if (child == -1) {
		perror("fork() Error");
	} else if (child == 0) {
		if (filename != NULL) {
			int fd1;
			if (std_fd == STDOUT_FILENO) {
				fd1 = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
			} else if (std_fd == STDIN_FILENO) {
				fd1 = open(filename, O_RDONLY, 0644);
			}

			if (fd1 == -1) {
				perror("open() Error");
				return;
			}

			int new_fd = dup2(fd1, std_fd);

			if (new_fd == -1) {
				perror("dup2() Error");
				return;
			}

			if (close(fd1) == -1) {
				perror("close() Error");
				return;
			}
		}
		
		execvp(args[0], args);
		perror("execvp() Error");
		exit(1);	
	} else {
		waitpid(child, NULL, 0);
	}

	return;
}



int main() {
	char in_buff[128];
	char *args[64];
	size_t size_inbuff = 128;
	size_t size_args = 64;
	char cur_path[1024];

	while (1) {
		if (getcwd(cur_path, sizeof(cur_path)) != NULL) {
		printf("(%s)#: ", cur_path);
		fflush(stdout);

		} else {
			perror("getcwd() Error");
			printf("(?)#: ");
			fflush(stdout);
		}

		if (input(in_buff, size_inbuff) == -1) {
			printf("input() Error");
			break;
		}

		int tokenizing_ret = tokenizing(in_buff, args, size_args);

		if (tokenizing_ret == 1) {
			break;
		} else if (tokenizing_ret == 2) {
			continue;
		} else if (tokenizing_ret == -1) {
			return -1;
		}
		
		execute(args, NULL, -1);
	}

	return 0;
}

