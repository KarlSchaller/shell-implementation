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

void printprompt();
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
		
		printprompt();
		
		size_t linesize = 32;
		char *line = (char *)malloc(linesize * sizeof(char));
		size_t len = getline(&line, &linesize, stdin);
		line[len-1] = '\0';
		
		if (strcmp(line, "quit") == 0 || strcmp(line, "^D") == 0)
			exit(0);
			
		char **argv2 = (char **)malloc((len+1)*sizeof(char *));
		argv2[0] = strtok(line, " ");
		int argc2 = 0;
		for (; argv2[argc2] != NULL; argc2++)
			argv2[argc2+1] = strtok(NULL, " ");
			
			
		char *command = argv2[0];
		int pid;
		
		
		// Built-In commands
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
		
		
		// External commands (PIPING AND REDIRECTION DO NOT WORK AT THE SAME TIME)
		else if ((pid = fork()) < 0)
			perror("Forking Error");
		else if (pid == 0) {	
			// TODO OUTPUT REDIRECTION SHOULD WORK WITH DIR, ENVIRON, ECHO, AND HELP TOO BUT NOT INPUT REDIRECTION
			int i;
			
			// Piping
			if ((i = argsearch(argv2, "|")) != -1) {
				int p[2];
				if (pipe(p) < 0) {
					perror("Pipe Error");
					exit(1);
				}
				if ((pid = fork()) < 0)
					perror("Forking Error");
				else if (pid == 0) {
					close(p[0]);
					close(STDOUT_FILENO);
					dup2(p[1], STDOUT_FILENO);
					close(p[1]);
					argv2[i] = NULL;
				}
				else {
					close(p[1]);
					close(STDIN_FILENO);
					dup2(p[0], STDIN_FILENO);
					close(p[0]);
					argv2 += (i+1)*sizeof(char *); // does this work?
				}
			}
			
			// Input redirection
			if ((i = argsearch(argv2, "<")) != -1) {
				int infile = open(argv2[i+1], O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
				close(STDIN_FILENO);
				dup2(infile, STDIN_FILENO);
				close(infile);
			}
			
			// Output redirection
			if ((i = argsearch(argv2, ">")) != -1) {
				int outfile = open(argv2[i+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
				close(STDOUT_FILENO);
				dup2(outfile, STDOUT_FILENO);
				close(outfile);
			}
			else if ((i = argsearch(argv2, ">>")) != -1) {
				int outfile = open(argv2[i+1], O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
				close(STDOUT_FILENO);
				dup2(outfile, STDOUT_FILENO);
				close(outfile);
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

// Print the current working directory
void printprompt() {
	char current_dir[1024];
	getcwd(current_dir, 1024);
	if (current_dir)
		printf("%s> ", current_dir);
	else
		printf("ERROR> ");
}

// Change the directory
void cd(char **argv) {
	if (argv[1] == NULL && chdir(getenv("HOME")) != 0)
		perror("Could not change directory");
	else if (chdir(argv[1]) != 0)
		perror("Could not change directory");
}
	
// Clear the screen
void clr() {
	printf("\33[H\033[2J");
}
	
// Print the contents of the current directory or a user-specified directory
void dir(char **argv) {
	// TODO
	DIR *dir;
	if (argv[1] == NULL || strcmp(argv[1], ">") == 0 || strcmp(argv[1], ">>") == 0)
		dir = opendir("./");
	else
		dir = opendir(argv[1]);
		
	struct dirent *s;
	while ((s = readdir(dir)) != NULL)
		printf("%s\n", s->d_name);
}

// List the environment variables
void environ(char **envp) {
	for (int i = 0; envp[i] != NULL; i++)
		printf("%s\n", envp[i]);
}
	
// Print the user's input	
void echo() {
	size_t linesize = 32;
	char *line = (char *)malloc(linesize * sizeof(char));
	getline(&line, &linesize, stdin);
	printf("%s", line);
	free(line);
}
	
// Display the user manual
void help() {
	puts("manual file");
}
	
// Pause the shell until the user presses Enter
void mypause() {
	while (getchar() != '\n')
		fflush(stdin);
}

// Consider input to be program invocation
void execute(char **argv) {
	if (execvp(argv[0], argv) < 0) {
        perror("Execute Error");
		exit(1);
    }
}

// Search argv for key and return index or -1 if not found
int argsearch(char **argv, char *key) {
	for (int i = 0; argv[i] != NULL; i++) {
		if (strcmp(argv[i], key) == 0)
			return i;
	}
	return -1;
}