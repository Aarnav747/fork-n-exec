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

int tokenizing(char in_buff[], char *args[], size_t size_args) {
	args[0] = strtok(in_buff, " \n");

	if (strcmp(args[0], "exit") == 0) return 1;

	int i = 1;
	for (i = 1; i < size_args; i++) {
		char *token = strtok(NULL, " \n");

		if (token == NULL) break; 

		args[i] = token;
	}
	args[i] = NULL;

	return 0;
}

int inbuilt_inst(char *args[]) {
	if (strcmp(args[0], "cd") == 0) {
		printf("john");
		if (args[1] == NULL) return -1;

		if (chdir(args[1]) == -1) return -1;
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

	while (1) {
		printf("#: ");
		fflush(stdout);

		if (input(in_buff, size_inbuff) == -1) {
			printf("input() Error");
			break;
		}

		if (tokenizing(in_buff, args, size_args) == 1) break;

		if (inbuilt_inst(args) == -1) {
			printf("inbuilt_inst() Error");
			break;
		}

		execute(args);
	}

	return 0;
}

