/*
============================================================================
Name        : Karl Schaller
Date        : 03/16/2020
Course      : CIS3207
Homework    : Assignment 2 Shell Implementation
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>

void cd(char **argv);
void clr();
void dir(char **argv);
void environ(char **envp);
void echo();
void help();
void mypause();
void execute(char **argv);
int argsearch(char **argv, char *key);

void main(int argc, char **argv, char** envp) {

	if (argc > 1) {
		int infile = open(argv[1], O_RDONLY);
		dup2(infile, STDIN_FILENO);
	}
	
	while (1) {
		
		printf("myshell> ");
		
		size_t linesize = 32;
		char *line = (char *)malloc(linesize * sizeof(char));
		size_t len = getline(&line, &linesize, stdin);
		line[len-1] = '\0';
		
		if (strcmp(line, "exit") == 0 || strcmp(line, "^D") == 0)
			exit(0);
			
		char **argv2 = (char **)malloc((len+1)*sizeof(char *));
		argv2[0] = strtok(line, " ");
		int argc2 = 0;
		for (; argv2[argc2] != NULL; argc2++)
			argv2[argc2+1] = strtok(NULL, " ");
			
			
		char *command = argv2[0];
		
		
		// Built in commands
		if (strcmp(command, "cd") == 0)
			cd(argv2);
		else if (strcmp(command, "clr") == 0)
			clr();
		else if (strcmp(command, "dir") == 0)
			dir(argv2);
		else if (strcmp(command, "environ") == 0)
			environ(envp);
		else if (strcmp(command, "echo") == 0)
			echo();
		else if (strcmp(command, "help") == 0)
			help();
		else if (strcmp(command, "pause") == 0)
			mypause();
		
		
		// External commands
		else if (fork() == 0) {	// run external command
			// TODO
			int i;
			if ((i = argsearch(argv2, "|")) != -1) {
				int p[2];
				if (pipe(p) < 0) {
					perror("Pipe error");
					exit(1);
				}
				if (fork() == 0) {
					dup2(p[1], STDOUT_FILENO);
					argv2[i] = NULL;
				}
				else {
					dup2(p[0], STDIN_FILENO);
					argv2 = argv2 + (i+1)*sizeof(char *); // does this work?
				}
			}
			if ((i = argsearch(argv2, "<")) != -1) {
				int infile = open(argv2[i+1], O_RDONLY);
				dup2(infile, STDIN_FILENO);
			}
			if ((i = argsearch(argv2, ">")) != -1) {
				int outfile = open(argv2[i+1], O_WRONLY | O_TRUNC | O_CREAT);
				dup2(outfile, STDOUT_FILENO);
			}
			else if ((i = argsearch(argv2, ">>")) != -1) {
				int outfile = open(argv2[i+1], O_WRONLY | O_APPEND | O_CREAT);
				dup2(outfile, STDOUT_FILENO);
			}
			execute(argv2);
		}
		else if (argsearch(argv2, "&") == -1) { // wait for external command to finish TODO
			int status = 0;
			wait(&status);
			printf("Child exited with status of %d\n", status);
		}
		
		
		free(line);
		free(argv2);
	}
}

// Change the directory
void cd(char **argv) {
	// TODO
	if (argv[1] != NULL && strcmp(argv[1], "</>/>>") == 0) {
		if (chdir(argv[1]) != 0)
			perror("no such dir");
	}
	else
		chdir(getenv("HOME"));
}
	
// Clear the screen
void clr() {
	printf("\33[H\033[2J");
}
	
// Print the contents of the current directory or a user-specified directory
void dir(char **argv) {
	// TODO
	DIR *dir;
	if (argv[1] != NULL && strcmp(argv[1], "</>/>>") != 0)
		dir = opendir(argv[1]);
	else
		dir = opendir("./");
	struct dirent *s;
	while ((s = readdir(dir)) != NULL)
		printf("%s\t", s->d_name);
}

// List the environment variables
void environ(char **envp) {
	for (int i = 0; envp[i] != NULL; i++)
		printf(envp[i]);
}
	
// Print the user's input	
void echo() {
	size_t linesize = 32;
	char *line = (char *)malloc(linesize * sizeof(char));
	getline(&line, &linesize, stdin);
	printf(line);
	free(line);
}
	
// Display the user manual
void help() {
	printf("manual file");
}
	
// Pause the shell until the user presses Enter
void mypause() {
	while (getchar() != '\n')
		fflush(stdin);
}

// Consider input to be program invocation
void execute(char **argv) {
	// TODO
	char *command = argv[0];
	
	// Look for file in current directory
	int fd = open(command, O_RDONLY);
	if (fd != -1)
		execvp(command, argv);
	
	// Look for file in path
	char *path = getenv("PATH");
	char *paths[strlen(path)];
	paths[0] = strtok(path, ":");
	for (int i = 0; paths[i] != NULL; i++)
		paths[i+1] = strtok(NULL, ":");
	
	for (int i = 0; paths[i] != NULL; i++) {
		char *dir = strcat(paths[i], command);
		fd = open(dir, O_RDONLY);
		if (fd != -1)
			execvp(dir, argv);
	}
	
	printf("file not found");
}

// Search argv for key and return index or -1 if not found
int argsearch(char **argv, char *key) {
	for (int i = 0; argv[i] != NULL; i++) {
		if (strcmp(argv[i], key) == 0)
			return i;
	}
	return -1;
}