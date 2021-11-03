#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// fork()
#include <unistd.h>

// waitpid()
#include <sys/types.h> 
#include <sys/wait.h> 


/*
 * ESH - Ethan's Shell:
 *
 * A basic interactive shell--no scripting--for personal
 * learning experience.
 *
 */

// Built-ins.

int cd(const char* filename)
{
	if (chdir(filename) == 0) {
		return 0;
	} else if (errno == ENOTDIR) {
		printf("%s is not a directory.\n", filename);
	} else if (errno == EACCES) {
		printf("The process does not have search permission for directory.\n");
	} else if (errno == ENAMETOOLONG) {
		printf("Directory name is too long.\n");
	} else if (errno == ENOENT) {
		printf("Directory does not exist.\n");
	} else if (errno == ELOOP) {
		printf("Too many symbolic links were resolved while trying to look up the directory.\n");
	}

	return -1;
}

char* pwd()
{
	char* cwd;
	char buff[256];

	cwd = getcwd(buff, 256);
	if (cwd != NULL) {
		return cwd;
	} else {
		return NULL;
	}
}


// Environment variables.
extern char **environ;


void signalHandler(int sigNum)
{
	if (sigNum == SIGINT) {
		//printf("recieved SIGINT, exiting...\n");
		exit(0);
	}

}

char** splitString (char* str, char* delim)
{
	char** sStr = (char**)malloc(256 * sizeof(char**));

	if (sStr == NULL) {
		printf("splitString(): could not allocate memory for sStr, exiting...");
		exit(-1);
	}

	int tokenIndex = 0;

	// strtok expects string as intial first argument, subsequent calls expects NULL;
	char *token;
		
	token = strtok(str, delim);

	// Iterate over all tokens, or until max amount of loops is reached.
	while (token != NULL) {
		if (tokenIndex > 256) {
			break;
		}

		sStr[tokenIndex] = token;
		tokenIndex = tokenIndex + 1;
		token = strtok(NULL, delim);
	}

	// Add null terminator so a while loop can find end of list. 
	sStr[tokenIndex] = NULL;

	// Resize pointer to fit final string.
	size_t len = (tokenIndex + 1) * sizeof(char**);
	char** stringDestination = realloc(sStr, len);
	
	if (stringDestination == NULL) {
		printf("splitString(): could not copy memory to stringDestination, exiting...");
		free(sStr);
		exit(-1);
	}

	return stringDestination;
}

void removeTrailingNewline(char * str)
{
	if (str == NULL) {
		return;
	}

	int len = strlen(str);
	if (str[len-1] == '\n') {
		str[len-1] = '\0';
	}
}

pid_t spawnChild(char* file, char** args, char *const env[])
{
	// Check if there are no args...
	
	removeTrailingNewline(file);

	// Get length of arguments list.
	int argsLen = 0;
	while(args[argsLen] != NULL) {
		if (argsLen > 256) {
			break;
		}

		argsLen++;
	}

	if (argsLen > 1) {
	}

	char program[64] = "/usr/bin/";
	strncat(program, file, 64);

	pid_t childPid = fork();

	if (childPid == -1) {
		printf("fork() failed to create child process, exiting...");
		exit(-1);
	} else if (childPid > 0) {
		//printf("Child with pid %d was created\n", childPid);
		return childPid;
	} else {

		// Code is executed by child process.
		execve(program, args, env);

		// execvp returns only when an error occures.
		printf("excve in child process returned an error.\n");
		exit(-1);
	}
}

int runChild(char* buffer)
{
		// Command with arguments from stdin go here;
		// Think of this as the argv parameter for the child process's  main() function.
		// Make sure to resize this array after setting its values.
		char*  childArgv[256] = {};
		char** splitStr = splitString(buffer, " ");
		int j = 0;

		while (splitStr[j] != NULL) {
			removeTrailingNewline(splitStr[j]);		
			childArgv[j] = splitStr[j];
			j++;
		}

		free(splitStr);

		pid_t child;

		// Check if array has commands.
		if (*childArgv[0] == '\n' || *childArgv[0] == 0) {
			// No command was entered.
			
			return 0;
		} else if (strcmp(childArgv[0], "clear") == 0) {
			// Clear the console.
			system("clear");
		} else if (strcmp(childArgv[0], "cd") == 0) {
			// Use esh's cd built-in.
			//printf("cd entered!\n");
			
			removeTrailingNewline(childArgv[1]);

			cd(childArgv[1]);
			
			return 0;
		} else if (strcmp(childArgv[0], "exit") == 0) {
			//printf("exit entered!\n");

			// Exit shell, free allocated memory.
			free(buffer);
			exit(0);
		} else {
			int status;
	
			child = spawnChild(childArgv[0], childArgv, environ);

			// Suspends parent process until child process ends or is stopped.
			if (waitpid(child, NULL, 0) == -1) {

				printf("Failed to execute program in child process.\n");
				free(buffer);
				exit(-1);
			}

			if (WIFEXITED(status)) {
				// Exited normally.
				// printf("Child exited, status=%d\n", WEXITSTATUS(status));
			}

			if (WIFSIGNALED(status)) {
				// Child was killed, usually by a signal.
				// printf("Child killed, status=%d\n", WTERMSIG(status));
			}

			if (WIFSTOPPED(status)) {
				// Child was stopped.
				// printf("Child stopped, status=%d\n", WSTOPSIG(status));
			}

			if (WIFCONTINUED(status)) {
				//printf("Child continued from job control stop");
			}
			
			//free(buffer);
		}

		return 0;
}

void processLine()
{

	// Maximum characters the user can type into console; number is arbitrary.
	size_t buffer_size = 100;

	/*char* env = getenv("PATH");

	if (env == NULL) {
		printf("getenv() couldn't get environment variable.\n");
	} else {
		printf("%s\n", env);
	}*/

	while(1) {

		// Get current directory.
		char* cwd;
		char buff[256];

		cwd = getcwd(buff, 256);
		if (cwd == NULL) {
			printf("getcwd() error, exiting...");
			exit(-1);
		}

		// Print the prompt.
		printf("\x1B[34m%s $\033[0m ", cwd);

		// Clear output "stdout" buffer, forces output to write to console in kernel buffer.
		if (fflush(stdout) == EOF) {
			// errno is not kept by fflush, so we store it for later use.
			//int err = errno;
			//if (err == )
			
			printf("fflush(stdout) failed, exiting...");
			exit(-1);
		}

		// Allocate memory for input buffer , e.i. the characters typed into the console.
		char *buffer = NULL;

		// Allocate memory for input buffer , e.i. the characters typed into the console.
		buffer = (char *)malloc(buffer_size * sizeof(char));

		if (buffer == NULL) {
			printf("Could not allocate memory, exiting...");

			exit(-1);
			//return -1;
		}

		// Read current line of input to buffer;
		// If an error occurs or end of file is reached without any bytes read, getline returns -1.
		if (getline(&buffer, &buffer_size, stdin) == -1) {
			printf("Reached end of stdin");
			free(buffer);
			exit(0);
			break;
		}

		// Split stdin on :, && and ||.
		// If &&, run next command only if child exit status is 0.
		// If ||, run next command if status is not 0.
		
		/*char** commandsList;
		
		char*  commands[256] = {};
		char** splitCom = splitString(buffer, ";");
		int j = 0;

		while (splitStr[j] != NULL) {
			removeTrailingNewline(splitStr[j]);	
			runChild(splitCom[j]);
			j++;
		}

		free(splitCom);*/

		runChild(buffer);

		free(buffer);
	}
}


int main(int argc, char *argv)
{
	// Linux Signals handle async events being sent to program (e.g. Ctrl-C keystroke).

	// Ctrl-C
	if (signal(SIGINT, signalHandler) == SIG_ERR) {
		printf("Could not catch SIGINT signal.\n");
	}

	processLine();

	return 0;
}
