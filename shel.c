#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define IN_BUFF_SIZE 128
#define ARGS_SIZE 64
#define CUR_PATH_SIZE 1024

#define ERROR_BREAK_RET -1
#define EXIT_RET 1
#define CONTINUE_RET 2

typedef struct io_elements {
	int input_redir;
	int output_redir;
	char *filename;
	int std_fn;
} io_elements;

typedef struct pipe_elements {
	int contains_pipe;
	char *pipe_first_cmd[2];
	char *pipe_second_cmd[2];
} pipe_elements;

io_elements io_redir = {
	.input_redir = -1,
	.output_redir = -1,
	.filename = NULL,
	.std_fn = -1
};

pipe_elements pipe_struct = {
	.contains_pipe = -1
};

pid_t Fork() {
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork() Error");
		exit(1);
	}

	return pid;
}

void Execvp(char *arg_array[]) {
	execvp(arg_array[0], arg_array);
	perror("execvp() Error");
	_exit(1);
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

int arg_error_check(char* arg) {
	if (arg == NULL) {
		perror("Argument error");
		return CONTINUE_RET;
	}

	return 0;
}

int input(char in_buff[]) {
	memset(in_buff, 0, IN_BUFF_SIZE);

	char *fgets_ret = fgets(in_buff, IN_BUFF_SIZE, stdin);
	if (fgets_ret == NULL) return ERROR_BREAK_RET;

	return 0;
}	

void execute(char *args[]);

int inbuilt_cmds_flow(int case_no, char *args[], int i) {
	switch(case_no) {
		case 1:
			io_redir.output_redir = -1;
			io_redir.input_redir = -1;

			break;

		case 2: {
			char* first_cmd = args[i-1];
			char* second_cmd = args[i+1];

			if (arg_error_check(first_cmd) == CONTINUE_RET) {
				return CONTINUE_RET;
			} else if (arg_error_check(second_cmd) == CONTINUE_RET) return CONTINUE_RET;

			pipe_struct.pipe_first_cmd[0] = first_cmd;
			pipe_struct.pipe_first_cmd[1] = NULL;
			pipe_struct.pipe_second_cmd[0] = second_cmd;
			pipe_struct.pipe_second_cmd[1] = NULL;

			args[i] = NULL;
			args[i+2] = NULL;

			break;
		}

	}

	return 0;
}

int pipe_execute() {
	int pipefd[2];

	if (pipe(pipefd) == -1) {
		perror("pipe() Error");
		return ERROR_BREAK_RET;
	}

	pid_t pid1 = Fork();
	if (pid1 == 0) {
		Dup2(pipefd[1], STDOUT_FILENO);

		Close(pipefd[0]);
		Close(pipefd[1]);

		Execvp(pipe_struct.pipe_first_cmd);
	}

	pid_t pid2 = Fork();
	if (pid2 == 0) {
		Dup2(pipefd[0], STDIN_FILENO);

		Close(pipefd[1]);
		Close(pipefd[0]);

		Execvp(pipe_struct.pipe_second_cmd);
	} 

	Close(pipefd[0]);
	Close(pipefd[1]);
	
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);

	return CONTINUE_RET;
}

int tokenizing(char in_buff[], char *args[]) {
	memset(args, 0, ARGS_SIZE);

	args[0] = strtok(in_buff, " \n");

	if (args[0] == NULL) return -1;

	if (strcmp(args[0], "exit") == 0) {
		return EXIT_RET;
	}

	int i = 1;
	char *token = NULL;
	for (i = 1; i < ARGS_SIZE; i++) {
		token = strtok(NULL, " \n");

		if (token == NULL) break;

		args[i] = token;

		io_redir.output_redir = strcmp(token, ">");
		io_redir.input_redir = strcmp(token, "<");
		pipe_struct.contains_pipe = strcmp(token, "|");


		if (pipe_struct.contains_pipe == 0) {
			token = strtok(NULL, " \n");
			args[i+1] = token;

			break;

		} else if (io_redir.output_redir == 0 || io_redir.input_redir == 0) {
			token = strtok(NULL, " \n");
			if (arg_error_check(token) == 2) return CONTINUE_RET;
			io_redir.filename = token;
			args[i+1] = NULL;
			args[i] = NULL;

			break;
		} 
	}

	if (pipe_struct.contains_pipe == 0) {
		pipe_struct.contains_pipe = -1;

		if (inbuilt_cmds_flow(2, args, i) == 2) return CONTINUE_RET;

		int pipe_ex_ret = pipe_execute();
		return pipe_ex_ret;

	} else if (io_redir.output_redir == 0) {
		io_redir.std_fn = STDOUT_FILENO;
		inbuilt_cmds_flow(1, args, i);

		execute(args);
		return CONTINUE_RET;
	} else if (io_redir.input_redir == 0) {
		io_redir.std_fn = STDIN_FILENO;
		inbuilt_cmds_flow(1, args, i);

		execute(args);

		return CONTINUE_RET;
	}

	args[i] = NULL;

	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) return -1;

		if (chdir(args[1]) == -1) return -1;

		return CONTINUE_RET;
	}

	return 0;
}

void execute(char *args[]) {
	pid_t child = Fork();

	if (child == 0) {
	if (io_redir.filename != NULL) {
		int fd1;
		if (io_redir.std_fn == STDOUT_FILENO) {
			fd1 = open(io_redir.filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);

		} else if (io_redir.std_fn == STDIN_FILENO) {
			fd1 = open(io_redir.filename, O_RDONLY);
		}

		if (fd1 == -1) {
			perror("open() Error");
			exit(1);
			return;
		}

		int new_fd = Dup2(fd1, io_redir.std_fn);

		Close(fd1);
	}

		Execvp(args);
	} else {
		waitpid(child, NULL, 0);
	}
		
	return;
}

char* cur_dir_addr(char cur_path[]) {
	memset(cur_path, 0, CUR_PATH_SIZE);

	if (getcwd(cur_path, CUR_PATH_SIZE) != NULL) {
		char* last_slash = strrchr(cur_path, '/');
		if ((last_slash+1) == NULL) return NULL;

		return (last_slash + 1);
	} else {
		return NULL;
	}
}

struct sigaction sa;

void sigint_handler(int sig) {}

int main() {
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	char in_buff[IN_BUFF_SIZE];
	char *args[ARGS_SIZE];
	char cur_path[CUR_PATH_SIZE];
	char* cur_dir;

	while (1) {
		io_redir.filename = NULL;
		io_redir.std_fn = -1;

		cur_dir = cur_dir_addr(cur_path);

		if (cur_dir != NULL) {
			printf("(%s)#: ", cur_dir);
			fflush(stdout);

		} else if (cur_dir == NULL) {
			perror("getcwd() Error");
			printf("(?)#: ");
			fflush(stdout);
		}

		int input_func_ret = input(in_buff);

		if (input_func_ret == ERROR_BREAK_RET) {
			printf("input() Error");
			break;
		}

		if (in_buff[0] == '\0' || in_buff[0] == '\n') continue;

		int tokenizing_ret = tokenizing(in_buff, args);

		if (tokenizing_ret == 0) {
			execute(args);
		} else if (tokenizing_ret == CONTINUE_RET) {
			continue;
		} else if (tokenizing_ret == ERROR_BREAK_RET) {
			return -1;
		} else { 
			break;
		}
		
	}

	return 0;
}

