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
		case 1:
			if (args[1] == NULL) return -1;
			                                        		if (chdir(args[1]) == -1) return -1;										break;
									case 2:
			
	}
		                                                return 0;                               
}
					

int tokenizing(char in_buff[], char *args[], size_t size_args) {
	args[0] = strtok(in_buff, " \n");

	if (strcmp(args[0], "exit") == 0) {
		return 1;
	}

	int i = 1;
	for (i = 1; i < size_args; i++) {
		char *token = strtok(NULL, " \n");

		if (token == NULL) break;

		args[i] = token;

		if (token == ">" || token == "<") {
			inbuilt_inst(args, 2);
		}
	}
	args[i] = NULL;

	if (strcmp(args[0], "cd") == 0) {
		inbuilt_inst(args);
		return 2;
	}

	return 0;
}

void execute(char *args[]) {
	pid_t child = fork();

	if (child == -1) {
		perror("fork() Error");
	} else if (child == 0) {
		execvp(args[0], args);
		perror("execvp() Error");
		exit(1);
	} else {
		waitpid(child, NULL, 0);
	}
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

		} else perror("getcwd() Error");

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
		
		execute(args);
	}

	return 0;
}

