#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define IN_BUFF_SIZE 128
#define ARGS_SIZE 64
#define CUR_PATH_SIZE 1024

typedef struct inbuilt_commands {
	int input_redir;
	int output_redir;
	int contains_pipe;
} inbuilt_commands;

inbuilt_commands builtin_check;

pid_t Fork() {
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork() Error");
		exit(1);
	}

	return pid;
}

int Dup2(int fd_var, int std_fn) {
	int dup2_fd = dup2(fd_var, std_fn);

	if (dup2_fd == -1) {
		perror("dup2() Error");
		exit(1);
	}

	return dup2_fd;
}

void Close(int fd) {
	if (close(fd) == -1) {
		perror("close() Error");
		exit(1);
	}
}

int input(char in_buff[], size_t size_inbuff) {
	char *fgets_ret = fgets(in_buff, size_inbuff, stdin);
	if (fgets_ret == NULL) return -1;

	return 0;
}	

void execute(char *args[], char* filename, int std_fn);

int pipe_execute(char *pipe_out[], char *pipe_in[]) {
	int pipefd[2];

	if (pipe(pipefd) == -1) {
		perror("pipe() Error");
		return -1;
	}

	pid_t pid1 = Fork();
	if (pid1 == 0) {
		Dup2(pipefd[1], STDOUT_FILENO);

		Close(pipefd[0]);
		Close(pipefd[1]);

		execvp(pipe_out[0], pipe_out);
		perror("execvp() Error");
		exit(1);
	}

	pid_t pid2 = Fork();
	if (pid2 == 0) {
		Dup2(pipefd[0], STDIN_FILENO);

		Close(pipefd[1]);
		Close(pipefd[0]);

		execvp(pipe_in[0], pipe_in);
		perror("execvp() Error");
		exit(1);
	} 

	Close(pipefd[0]);
	Close(pipefd[1]);
	
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);

	return 2;
}

int tokenizing(char in_buff[], char *args[], size_t size_args) {
	args[0] = strtok(in_buff, " \n");

	if (strcmp(args[0], "exit") == 0) {
		return 1;
	}

	int i = 1;
	char *token = NULL;
	char *pipe_first_cmd[2];
	char *pipe_second_cmd[2];
	for (i = 1; i < size_args; i++) {
		token = strtok(NULL, " \n");

		if (token == NULL) break;

		args[i] = token;

		builtin_check.output_redir = strcmp(token, ">");
		builtin_check.input_redir = strcmp(token, "<");
		builtin_check.contains_pipe = strcmp(token, "|");


		if (builtin_check.contains_pipe == 0) {
			pipe_first_cmd[0] = args[i-1];
			pipe_first_cmd[1] = NULL;
			token = strtok(NULL, " \n");
			if (token == NULL) return -1;
			pipe_second_cmd[0] = token;
			pipe_second_cmd[1] = NULL;

			args[i] = NULL;
			args[i+2] = NULL;
			break;

		} else if (builtin_check.output_redir == 0 || builtin_check.input_redir == 0) {
			token = strtok(NULL, " \n");
			if (token == NULL) return -1;
			args[i+1] = NULL;
			args[i] = NULL;
			break;
		} 
	}

	if (builtin_check.output_redir == 0) {
		execute(args, token, STDOUT_FILENO);
		builtin_check.input_redir = -1;
		return 2;
	} else if (builtin_check.input_redir == 0) {
		execute(args, token, STDIN_FILENO);
		builtin_check.output_redir = -1;
		return 2;
	} else if (builtin_check.contains_pipe == 0) {
		if (pipe_first_cmd[0] == NULL) {
			printf("pipe_first_cmd is NULL");
			exit(1);
		} else if (pipe_second_cmd[0] == NULL) {
			printf("pipe_second_cmd is NULL");
			exit(1);
		}
		builtin_check.contains_pipe = -1;

		int pipe_ex_ret = pipe_execute(pipe_first_cmd, pipe_second_cmd);
		return pipe_ex_ret;
	}

	args[i] = NULL;

	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) return -1;

		if (chdir(args[1]) == -1) return -1;

		return 2;
	}

	return 0;
}

void execute(char *args[], char* filename, int std_fn) {
	pid_t child = Fork();

	if (child == 0) {
	if (filename != NULL) {
		int fd1;
		if (std_fn == STDOUT_FILENO) {
			fd1 = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		} else if (std_fn == STDIN_FILENO) {
			fd1 = open(filename, O_RDONLY, 0644);
		}

		if (fd1 == -1) {
			perror("open() Error");
			exit(1);
			return;
		}

		int new_fd = Dup2(fd1, std_fn);

		Close(fd1);
	}

		printf("execvp in execute");
		execvp(args[0], args);
		perror("execvp() Error");
		exit(1);
	} else {
		waitpid(child, NULL, 0);
	}
		
	return;
}



int main() {
	char in_buff[IN_BUFF_SIZE];
	char *args[ARGS_SIZE];
	size_t size_inbuff = IN_BUFF_SIZE;
	size_t size_args = ARGS_SIZE;
	char cur_path[CUR_PATH_SIZE];
	builtin_check.input_redir = -1;
	builtin_check.output_redir = -1;
	builtin_check.contains_pipe = -1;

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

