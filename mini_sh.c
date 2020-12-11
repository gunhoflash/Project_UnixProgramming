/**
 * 서울시립대학교 컴퓨터과학부 2015920003 김건호
 * 2018-2 유닉스프로그래밍및실습 Term Project
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <wait.h>

#define FALSE 0
#define TRUE 1

#define EOL 1
#define ARG 2
#define AMPERSAND 3
#define LEFTBRACKET 4
#define RIGHTBRACKET 5
#define PIPE 6

#define FOREGROUND 0
#define BACKGROUND 1

static char input[512];
static char tokens[1024];
const char *homedir;

char *ptr, *tok;

int get_token(char **outptr)
{
	int type;

	*outptr = tok;
	while ((*ptr == ' ') || (*ptr == '\t')) ptr++;
	*tok++ = *ptr;

	switch (*ptr++)
	{
		case '\0' : type = EOL;          break;
		case '&'  : type = AMPERSAND;    break;
		case '<'  : type = LEFTBRACKET;  break;
		case '>'  : type = RIGHTBRACKET; break;
		case '|'  : type = PIPE;         break;
		default   : type = ARG;
			while ((*ptr != ' ') && (*ptr != '&') && (*ptr != '\t') && (*ptr != '\0'))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return type;
}

// Set input.
void set_input(pid_t *pid, int *fd, char *filepath)
{
	*pid = fork();
	if (*pid < 0)
	{
		// Handle Exception.
		perror("Error on fork() at set_input()");
		exit(-1);
	}
	else if (*pid == 0)
	{
		*fd = open(filepath, O_RDONLY);
		if (*fd < 0)
		{
			// Handle Exception.
			perror("Error on open() at set_input()");
			exit(-1);
		}
		if (dup2(*fd, STDIN_FILENO) < 0)
		{
			// Handle Exception.
			perror("Error on dup2() at set_input()");
			exit(-1);
		}
	}
}

// Set output.
void set_output(pid_t *pid, int *fd, char *filepath)
{
	*pid = fork();
	if (*pid < 0)
	{
		// Handle Exception.
		perror("Error on fork() at set_output()");
		exit(-1);
	}
	else if (*pid == 0)
	{
		*fd = open(filepath, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (*fd < 0)
		{
			// Handle Exception.
			perror("Error on open() at set_output()");
			exit(-1);
		}
		if (dup2(*fd, STDOUT_FILENO) < 0)
		{
			// Handle Exception.
			perror("Error on dup2() at set_output()");
			exit(-1);
		}
	}
}

void execute_with_io(char **comm, char *input, char *output)
{
	pid_t pid_execute, pid_input, pid_output;
	int fd_input, fd_output;

	pid_input = pid_output = 0;
	fd_input = fd_output = -1;

	// Set input and output.
	if (input) set_input(&pid_input, &fd_input, input);
	if (output) set_output(&pid_output, &fd_output, output);

	if (pid_input == 0 && pid_output == 0)
	{
		// Execute.

		if ((pid_execute = fork()) < 0)
			fprintf(stderr, "minish : fork error\n"); // Handle Exception.
		else if (pid_execute == 0)
		{
			execvp(*comm, comm);

			// Handle Exception.
			fprintf(stderr, "minish : command not found\n");
			exit(127);
		}
		else wait(NULL);

		// If needed, close and exit.
		if (fd_input >= 0) close(fd_input);
		if (fd_output >= 0) close(fd_output);
		if (input || output) exit(0);
		else return;
	}
	else if (input && output)
		if (!(pid_input == 0) != !(pid_output == 0))
			exit(0);
	wait(NULL);
}

// Separate input/output paths from array 'arg' and copy to input_path/output_path.
void get_io(int narg, char *arg[1024], int i_leftbracket, int i_rightbracket, char **input_path, char **output_path)
{
	int i, j;

	*input_path = NULL;
	*output_path = NULL;

	if (narg == 0) return;

	// Find the input path string.
	if (i_leftbracket >= 0)
	{
		if (narg > i_leftbracket + 1)
		{
			*input_path = arg[i_leftbracket + 1];
			arg[i_leftbracket + 1] = NULL;
		}
		arg[i_leftbracket] = NULL;
	}
	// Find the output path string.
	if (i_rightbracket >= 0)
	{
		if (narg > i_rightbracket + 1)
		{
			*output_path = arg[i_rightbracket + 1];
			arg[i_rightbracket + 1] = NULL;
		}
		arg[i_rightbracket] = NULL;
	}

	arg[narg] = NULL;

	// Trim arg
	for (i = 0; i < narg; i++)
	{
		if (arg[i] == NULL)
		{
			for (j = i + 1; j < narg; j++)
			{
				if (arg[j] != NULL)
				{
					arg[i] = arg[j];
					break;
				}
			}
			if (j == narg) break;
		}
	}
}

int execute(int narg, char *arg[1024], int i_leftbracket, int i_rightbracket, int i_pipe, int how)
{
	pid_t pid, pid_pipe;
	int pipes[2];
	char *inputs[2], *outputs[2];

	if ((pid = fork()) < 0)
	{
		// Handle Exception.
		perror("Error on fork() at execute()");
		return -1;
	}
	else if (pid == 0)
	{
		// Pipe needed.
		if (i_pipe >= 0)
		{
			pipe(pipes);
			if ((pid_pipe = fork()) < 0)
			{
				// Handle Exception.
				perror("Error on fork() at execute()");
				return -1;
			}
			else if (pid_pipe == 0)
			{
				// Set pipe-out.
				dup2(pipes[1], STDOUT_FILENO);
				close(pipes[0]);
				close(pipes[1]);
				get_io(i_pipe, arg, i_leftbracket, i_rightbracket, &inputs[0], &outputs[0]);
				execute_with_io(arg, inputs[0], outputs[0]);
				exit(0);
			}
			else
			{
				// Set pipe-in.
				dup2(pipes[0], STDIN_FILENO);
				close(pipes[0]);
				close(pipes[1]);
				get_io(narg - i_pipe - 1, &arg[i_pipe + 1], i_leftbracket, i_rightbracket, &inputs[1], &outputs[1]);
				execute_with_io(&arg[i_pipe + 1], inputs[1], outputs[1]);
				wait(NULL);
			}
		}
		else
		{
			// No pipe.
			get_io(narg, arg, i_leftbracket, i_rightbracket, &inputs[0], &outputs[0]);
			execute_with_io(arg, inputs[0], outputs[0]);
		}
		exit(0);
	}

	if (how == BACKGROUND)
	{
		/* Background execution */
		printf("[%d]\n", pid);
		return 0;
	}

	/* Foreground execution */
	while (waitpid(pid, NULL, 0) < 0)
		if (errno != EINTR)
			return -1;

	return 0;
}

int parse_and_execute(char *input)
{
	struct stat sb;
	char *arg[1024];
	int	type, how,
		quit           = FALSE,
		narg           = 0,
		finished       = FALSE,
		i_leftbracket  = -1, // Index of '<'
		i_rightbracket = -1, // Index of '>'
		i_pipe         = -1; // Index of '|'

	ptr = input;
	tok = tokens;
	while (!finished)
	{
		switch (type = get_token(&arg[narg]))
		{
			case ARG :
				narg++;
				break;

			/** 
			 * LEFTBRACKET, RIGHTBRACKET, PIPE :
			 * Catch only the first thing and ignore others.
			 * So, it cannot support multiple pipes.
			 */
			case LEFTBRACKET :
				if (i_leftbracket < 0)
					i_leftbracket = narg++;
				break;

			case RIGHTBRACKET :
				if (i_rightbracket < 0)
					i_rightbracket = narg++;
				break;

			case PIPE :
				if (i_pipe < 0)
					i_pipe = narg++;
				break;

			case EOL :
			case AMPERSAND :
				if (!strcmp(arg[0], "exit") || !strcmp(arg[0], "quit")) quit = TRUE;
				else if (!strcmp(arg[0], "cd"))
				{
					// Change '~' or '' to the HOME directory path.
					if (narg == 1 || !strcmp(arg[1], "~"))
						arg[1] = homedir;

					// Check validity of the path.
					if (stat(arg[1], &sb) == 0 && S_ISDIR(sb.st_mode))
						chdir(arg[1]); // Change directory.
					else
						printf("Cannot find directory: %s\n", arg[1]); // Handle Exception.
				}
				else
				{
					how = (type == AMPERSAND) ? BACKGROUND : FOREGROUND;
					arg[narg] = NULL;
					if (narg != 0)
						execute(narg, arg, i_leftbracket, i_rightbracket, i_pipe, how); // Execute.
				}

				narg = 0;
				if (type == EOL)
					finished = TRUE;
				break;
		}
	}
	return quit;
}

int main()
{
	// Get the path of HOME directory. 
	if ((homedir = getenv("HOME")) == NULL)
		homedir = getpwuid(getuid())->pw_dir;

	printf("msh # ");
	while (gets(input))
	{
		if (parse_and_execute(input)) break;
		printf("msh # ");
	}
}
