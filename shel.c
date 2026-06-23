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

typedef struct path_elements {
	char* cur_dir;
	char cur_path[CUR_PATH_SIZE];
} path_elements;

typedef struct args_elements {
	char *args[ARGS_SIZE];
	int cur_pos;
} args_elements;

io_elements io_redir = {
	.input_redir = -1,
	.output_redir = -1,
	.filename = NULL,
	.std_fn = -1
};

pipe_elements pipe_struct = {
	.contains_pipe = -1
};

path_elements path_struct = {
	.cur_dir = NULL
};

args_elements args_struct = {
	.cur_pos = 1
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

void execute();
char* cur_dir_addr();

int inbuilt_cmds_flow(int case_no) {
	switch(case_no) {
		case 1:
			io_redir.output_redir = -1;
			io_redir.input_redir = -1;

			break;

		case 2: {
			int i = args_struct.cur_pos;

			char* first_cmd = args_struct.args[i-1];
			char* second_cmd = args_struct.args[i+1];

			if (arg_error_check(first_cmd) == CONTINUE_RET) {
				return CONTINUE_RET;
			} else if (arg_error_check(second_cmd) == CONTINUE_RET) return CONTINUE_RET;

			pipe_struct.pipe_first_cmd[0] = first_cmd;
			pipe_struct.pipe_first_cmd[1] = NULL;
			pipe_struct.pipe_second_cmd[0] = second_cmd;
			pipe_struct.pipe_second_cmd[1] = NULL;

			args_struct.args[i] = NULL;
			args_struct.args[i+2] = NULL;

			break;
		}

		case 3: 
			if (args_struct.args[1] == NULL) return -1;

			if (chdir(args_struct.args[1]) == -1) return -1;

			path_struct.cur_dir = cur_dir_addr();

			return CONTINUE_RET;

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

int tokenizing(char in_buff[]) {
	memset(args_struct.args, 0, ARGS_SIZE);

	args_struct.args[0] = strtok(in_buff, " \n");

	if (args_struct.args[0] == NULL) return -1;

	if (strcmp(args_struct.args[0], "exit") == 0) {
		return EXIT_RET;
	}

	int i = 1;
	char *token = NULL;
	for (i = 1; i < ARGS_SIZE; i++) {
		token = strtok(NULL, " \n");

		if (token == NULL) break;

		args_struct.args[i] = token;

		io_redir.output_redir = strcmp(token, ">");
		io_redir.input_redir = strcmp(token, "<");
		pipe_struct.contains_pipe = strcmp(token, "|");


		if (pipe_struct.contains_pipe == 0) {
			token = strtok(NULL, " \n");
			args_struct.args[i+1] = token;
			args_struct.cur_pos = i;

			break;

		} else if (io_redir.output_redir == 0 || io_redir.input_redir == 0) {
			token = strtok(NULL, " \n");
			if (arg_error_check(token) == 2) return CONTINUE_RET;
			io_redir.filename = token;
			args_struct.args[i+1] = NULL;
			args_struct.args[i] = NULL;
			args_struct.cur_pos = i;

			break;
		} 
	}

	args_struct.args[i] = NULL;

	if (strcmp(args_struct.args[0], "cd") == 0) {
		if (inbuilt_cmds_flow(3) == CONTINUE_RET) return CONTINUE_RET;
	}

	return 0;
}

void execute() {
	if (pipe_struct.contains_pipe == 0) {
		pipe_struct.contains_pipe = -1;

		if (inbuilt_cmds_flow(2) == 2) return;

		pipe_execute();
		return;

	} else if (io_redir.output_redir == 0) {
		io_redir.std_fn = STDOUT_FILENO;

		inbuilt_cmds_flow(1);

	} else if (io_redir.input_redir == 0) {
		io_redir.std_fn = STDIN_FILENO;

		inbuilt_cmds_flow(1);

	}

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
		}

		int new_fd = Dup2(fd1, io_redir.std_fn);

		Close(fd1);
	}

		Execvp(args_struct.args);
	} else {
		waitpid(child, NULL, 0);
	}
		
	return;
}

char* cur_dir_addr() {
	memset(path_struct.cur_path, 0, CUR_PATH_SIZE);

	if (getcwd(path_struct.cur_path, CUR_PATH_SIZE) != NULL) {
		char* last_slash = strrchr(path_struct.cur_path, '/');
		if (last_slash == NULL) return NULL;

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

	path_struct.cur_dir = cur_dir_addr();

	while (1) {
		io_redir.filename = NULL;
		io_redir.std_fn = -1;

		if (path_struct.cur_dir != NULL) {
			printf("(%s)#: ", path_struct.cur_dir);
			fflush(stdout);

		} else {
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

		int tokenizing_ret = tokenizing(in_buff);

		if (tokenizing_ret == 0) {
			execute();
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

