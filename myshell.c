/*
============================================================================
Name        : Karl Schaller
Date        : 03/11/2020
Course      : CIS3207
Homework    : Assignment 2 Shell Implementation
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum event_type {JOB_ARRIVED, CPU_FINISHED, DISK1_FINISHED, DISK2_FINISHED, NETWORK_FINISHED, SIMULATION_FINISHED};

struct linked_list {
	struct node {
		union data {
			int job_num; // for cpu, etc. queue
			enum event_type type; // for priority queue
		} id;
		int time;
		struct node *next;
	} *head, *tail;
	int length;
} event_pq, cpu_q, disk1_q, disk2_q, network_q;

void enqueue(struct linked_list *q, int job_num, int time); // for fifo queue
void insert(struct linked_list *pq, enum event_type type, int time); // for event min pq
struct node pop(struct linked_list *q);

int main(int argc, char *argv[]) {
	
	
	// read txt config
	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL) {
		puts("eventsimulator: cannot open config");
		exit(1);
	}
	char buffer[32];
	const int SEED = atoi(fgets(buffer, 32, fp) + 5);
	const int INIT_TIME = atoi(fgets(buffer, 32, fp) + 10);
	const int FIN_TIME = atoi(fgets(buffer, 32, fp) + 9);
	const int ARRIVE_MIN = atoi(fgets(buffer, 32, fp) + 11);
	const int ARRIVE_MAX = atoi(fgets(buffer, 32, fp) + 11);
	const float QUIT_PROB = atof(fgets(buffer, 32, fp) + 10);
	const float NETWORK_PROB = atof(fgets(buffer, 32, fp) + 13);
	const int CPU_MIN = atoi(fgets(buffer, 32, fp) + 8);
	const int CPU_MAX = atoi(fgets(buffer, 32, fp) + 8);
	const int DISK1_MIN = atoi(fgets(buffer, 32, fp) + 10);
	const int DISK1_MAX = atoi(fgets(buffer, 32, fp) + 10);
	const int DISK2_MIN = atoi(fgets(buffer, 32, fp) + 10);
	const int DISK2_MAX = atoi(fgets(buffer, 32, fp) + 10);
	const int NETWORK_MIN = atoi(fgets(buffer, 32, fp) + 12);
	const int NETWORK_MAX = atoi(fgets(buffer, 32, fp) + 12);
	fclose(fp);
	srand(SEED);
	fp = fopen("log.txt", "w");
	if (fp == NULL) {
		puts("eventsimulator: cannot open log");
		exit(1);
	}
	fprintf(fp, "SEED %d\n", SEED);
	fprintf(fp, "INIT_TIME %d\n", INIT_TIME);
	fprintf(fp, "FIN_TIME %d\n", FIN_TIME);
	fprintf(fp, "ARRIVE_MIN %d\n", ARRIVE_MIN);
	fprintf(fp, "ARRIVE_MAX %d\n", ARRIVE_MAX);
	fprintf(fp, "QUIT_PROB %f\n", QUIT_PROB);
	fprintf(fp, "NETWORK_PROB %f\n", NETWORK_PROB);
	fprintf(fp, "CPU_MIN %d\n", CPU_MIN);
	fprintf(fp, "CPU_MAX %d\n", CPU_MAX);
	fprintf(fp, "DISK1_MIN %d\n", DISK1_MIN);
	fprintf(fp, "DISK1_MAX %d\n", DISK1_MAX);
	fprintf(fp, "DISK2_MIN %d\n", DISK2_MIN);
	fprintf(fp, "DISK2_MAX %d\n", DISK2_MAX);
	fprintf(fp, "NETWORK_MIN %d\n", NETWORK_MIN);
	fprintf(fp, "NETWORK_MAX %d\n", NETWORK_MAX);
	
	
	// statistics
	double event_re_total = 0, cpu_re_total = 0, disk1_re_total = 0, disk2_re_total = 0, network_re_total = 0;
	int event_max = 0, cpu_max = 0, disk1_max = 0, disk2_max = 0, network_max = 0;
	double cpu_busy = 0, disk1_busy = 0, disk2_busy = 0, network_busy = 0;
	int cpu_re_max = 0, disk1_re_max = 0, disk2_re_max = 0, network_re_max = 0;
	double cpu_jobs = 0, disk1_jobs = 0, disk2_jobs = 0, network_jobs = 0;
	
	// add arrival of first event
	int job_num = 1;
	insert(&event_pq, JOB_ARRIVED, INIT_TIME);
	insert(&event_pq, SIMULATION_FINISHED, FIN_TIME);
	
	while (event_pq.length > 0) { // will go until SIMULATION_FINISHED event
		
		// read event
		if (event_pq.length > event_max)
			event_max = event_pq.length;
		struct node event = pop(&event_pq);
		
		// handle event
		struct node job;
		int response, duration;
		switch (event.id.type) {
			
			case JOB_ARRIVED :
				fprintf(fp, "At time %d, Job %d arrives.\n", event.time, job_num);
				
				// send job to cpu
				enqueue(&cpu_q, job_num++, event.time);
				if (cpu_q.length == 1) {
					duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
				    insert(&event_pq, CPU_FINISHED, duration + event.time);
					cpu_busy += duration;
					event_re_total += duration;
					fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, job_num-1);
				}
				
				// determine next arrival
				duration = rand() % (ARRIVE_MAX-ARRIVE_MIN+1) + ARRIVE_MIN;
				insert(&event_pq, JOB_ARRIVED, duration + event.time);
				event_re_total += duration;
				break;
				
			case CPU_FINISHED : ;
				job = pop(&cpu_q);
				fprintf(fp, "At time %d, Job %d finishes in the CPU.\n", event.time, job.id.job_num);
				
				//update statistics
				response = event.time-job.time;
				cpu_re_total += response;
				if (cpu_q.length+1 > cpu_max)
					cpu_max = cpu_q.length+1;
				if (response > cpu_re_max)
					cpu_re_max = response;
				cpu_jobs++;
				
				// job exits system
				int prob = rand() % 100;
				if (prob < 100*QUIT_PROB) {
					fprintf(fp, "At time %d, Job %d exits the system.\n", event.time, job.id.job_num);
				    if (cpu_q.length >= 1) {
						fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, cpu_q.head->id.job_num);
						duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
					    insert(&event_pq, CPU_FINISHED, duration + event.time);
						cpu_busy += duration;
						event_re_total += duration;
					}
					break;
				}
				
				// job uses network
				prob = rand() % 100;
				if (prob < 100*NETWORK_PROB) {
					enqueue(&network_q, job.id.job_num, event.time);
					if (network_q.length == 1) {
						fprintf(fp, "At time %d, Job %d enters the Network.\n", event.time, job.id.job_num);
						duration = rand() % (NETWORK_MAX-NETWORK_MIN+1) + NETWORK_MIN;
						insert(&event_pq, NETWORK_FINISHED, duration + event.time);
						network_busy += duration;
						event_re_total += duration;
					}
				}
				
				// job does disk read
				else if (disk1_q.length < disk2_q.length) {
					enqueue(&disk1_q, job.id.job_num, event.time);
					if (disk1_q.length == 1) {
						fprintf(fp, "At time %d, Job %d enters Disk 1.\n", event.time, job.id.job_num);
						duration = rand() % (DISK1_MAX-DISK1_MIN+1) + DISK1_MIN;
						insert(&event_pq, DISK1_FINISHED, duration + event.time);
						disk1_busy += duration;
						event_re_total += duration;
					}
				}
				else if (disk1_q.length > disk2_q.length) {
					enqueue(&disk2_q, job.id.job_num, event.time);
					if (disk2_q.length == 1) {
						fprintf(fp, "At time %d, Job %d enters Disk 2.\n", event.time, job.id.job_num);
						duration = rand() % (DISK2_MAX-DISK2_MIN+1) + DISK2_MIN;
						insert(&event_pq, DISK2_FINISHED, duration + event.time);
						disk2_busy += duration;
						event_re_total += duration;
					}
				}
				else {
					prob = rand() % 2;
					if (prob) {
						enqueue(&disk1_q, job.id.job_num, event.time);
						if (disk1_q.length == 1) {
							fprintf(fp, "At time %d, Job %d enters Disk 1.\n", event.time, job.id.job_num);
							duration = rand() % (DISK1_MAX-DISK1_MIN+1) + DISK1_MIN;
							insert(&event_pq, DISK1_FINISHED, duration + event.time);
							disk1_busy += duration;
							event_re_total += duration;
						}
					}
					else {
						enqueue(&disk2_q, job.id.job_num, event.time);
						if (disk2_q.length == 1) {
							fprintf(fp, "At time %d, Job %d enters Disk 2.\n", event.time, job.id.job_num);
							duration = rand() % (DISK2_MAX-DISK2_MIN+1) + DISK2_MIN;
							insert(&event_pq, DISK2_FINISHED, duration + event.time);
							disk2_busy += duration;
							event_re_total += duration;
						}
					}
				}
				
				// check for next job
				if (cpu_q.length >= 1) {
					fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, cpu_q.head->id.job_num);
					duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
				    insert(&event_pq, CPU_FINISHED, duration + event.time);
					cpu_busy += duration;
					event_re_total += duration;
				}
				break;
				
			case DISK1_FINISHED : ;
				job = pop(&disk1_q);
				fprintf(fp, "At time %d, Job %d finishes in Disk 1.\n", event.time, job.id.job_num);
				
				//update statistics
				response = event.time-job.time;
				disk1_re_total += response;
				if (disk1_q.length+1 > disk1_max)
					disk1_max = disk1_q.length+1;
				if (response > disk1_re_max)
					disk1_re_max = response;
				disk1_jobs++;
				
				// job goes to cpu
				enqueue(&cpu_q, job.id.job_num, event.time);
				if (cpu_q.length == 1) {
					fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, job.id.job_num);
					duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
				    insert(&event_pq, CPU_FINISHED, duration + event.time);
					cpu_busy += duration;
					event_re_total += duration;
				}
				
				// check for next job
				if (disk1_q.length >= 1) {
					fprintf(fp, "At time %d, Job %d enters Disk 1.\n", event.time, disk1_q.head->id.job_num);
					duration = rand() % (DISK1_MAX-DISK1_MIN+1) + DISK1_MIN;
					insert(&event_pq, DISK1_FINISHED, duration + event.time);
					disk1_busy += duration;
					event_re_total += duration;
				}
				break;
				
			case DISK2_FINISHED : ;
				job = pop(&disk2_q);
				fprintf(fp, "At time %d, Job %d finishes in Disk 2.\n", event.time, job.id.job_num);
				
				//update statistics
				response = event.time-job.time;
				disk2_re_total += response;
				if (disk2_q.length+1 > disk2_max)
					disk2_max = disk2_q.length+1;
				if (response > disk2_re_max)
					disk2_re_max = response;
				disk2_jobs++;
				
				// job goes to cpu
				enqueue(&cpu_q, job.id.job_num, event.time);
				if (cpu_q.length == 1) {
					fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, job.id.job_num);
					duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
				    insert(&event_pq, CPU_FINISHED, duration + event.time);
					cpu_busy += duration;
					event_re_total += duration;
				}
				
				// check for next job
				if (disk2_q.length >= 1) {
					fprintf(fp, "At time %d, Job %d enters Disk 2.\n", event.time, disk2_q.head->id.job_num);
					duration = rand() % (DISK2_MAX-DISK2_MIN+1) + DISK2_MIN;
					insert(&event_pq, DISK2_FINISHED, duration + event.time);
					disk2_busy += duration;
					event_re_total += duration;
				}
				break;
				
			case NETWORK_FINISHED : ;
				job = pop(&network_q);
				fprintf(fp, "At time %d, Job %d finishes in the Network.\n", event.time, job.id.job_num);
				
				//update statistics
				response = event.time-job.time;
				network_re_total += response;
				if (network_q.length+1 > network_max)
					network_max = network_q.length+1;
				if (response > network_re_max)
					network_re_max = response;
				network_jobs++;
				
				// job goes to cpu
				enqueue(&cpu_q, job.id.job_num, event.time);
				if (cpu_q.length == 1) {
					fprintf(fp, "At time %d, Job %d enters the CPU.\n", event.time, job.id.job_num);
					duration = rand() % (CPU_MAX-CPU_MIN+1) + CPU_MIN;
				    insert(&event_pq, CPU_FINISHED, duration + event.time);
					cpu_busy += duration;
					event_re_total += duration;
				}
				
				// check for next job
				if (network_q.length >= 1) {
					fprintf(fp, "At time %d, Job %d enters the Network.\n", event.time, network_q.head->id.job_num);
					duration = rand() % (NETWORK_MAX-NETWORK_MIN+1) + NETWORK_MIN;
					insert(&event_pq, NETWORK_FINISHED, duration + event.time);
					network_busy += duration;
					event_re_total += duration;
				}
				break;
				
			case SIMULATION_FINISHED :
				fprintf(fp, "At time %d, simulation finished.\n", event.time);
				
				int sim_time = FIN_TIME-INIT_TIME;
				puts("EVENT HANDLER:\n========================");
				printf("Average Queue Size: %f\n", event_re_total/sim_time);
				printf("Maximum Queue Size: %d\n", event_max);
				puts("\nCPU:\n========================");
				printf("Average Queue Size: %f\n", cpu_re_total/sim_time);
				printf("Maximum Queue Size: %d\n", cpu_max);
				printf("Ulitilization: %f\n", cpu_busy/sim_time);
				printf("Average Response Time: %f\n", cpu_re_total/cpu_jobs);
				printf("Maximum Response Time: %d\n", cpu_re_max);
				printf("Throughput: %f\n", cpu_jobs/sim_time);
				puts("\nDISK1:\n========================");
				printf("Average Queue Size: %f\n", disk1_re_total/sim_time);
				printf("Maximum Queue Size: %d\n", disk1_max);
				printf("Ulitilization: %f\n", disk1_busy/sim_time);
				printf("Average Response Time: %f\n", disk1_re_total/disk1_jobs);
				printf("Maximum Response Time: %d\n", disk1_re_max);
				printf("Throughput: %f\n", disk1_jobs/sim_time);
				puts("\nDISK2:\n========================");
				printf("Average Queue Size: %f\n", disk2_re_total/sim_time);
				printf("Maximum Queue Size: %d\n", disk2_max);
				printf("Ulitilization: %f\n", disk2_busy/sim_time);
				printf("Average Response Time: %f\n", disk2_re_total/disk2_jobs);
				printf("Maximum Response Time: %d\n", disk2_re_max);
				printf("Throughput: %f\n", disk2_jobs/sim_time);
				puts("\nNETWORK:\n========================");
				printf("Average Queue Size: %f\n", network_re_total/sim_time);
				printf("Maximum Queue Size: %d\n", network_max);
				printf("Ulitilization: %f\n", network_busy/sim_time);
				printf("Average Response Time: %f\n", network_re_total/network_jobs);
				printf("Maximum Response Time: %d\n", network_re_max);
				printf("Throughput: %f\n", network_jobs/sim_time);
				
				fclose(fp);
				exit(0);
		}
	}
}

void enqueue(struct linked_list *q, int job_num, int time) {
	struct node *new_node = (struct node *) malloc(sizeof(struct node));
	new_node->id.job_num = job_num;
	new_node->time = time;
	new_node->next = NULL;
	if (q->head == NULL) {
		q->head = new_node;
		q->tail = new_node;
		q->length = 1;
	}
	else {
		q->tail->next =  new_node;
		q->tail = q->tail->next;
		q->length++;
	}
}

void insert(struct linked_list *pq, enum event_type type, int time) {
	struct node *new_node = (struct node *) malloc(sizeof(struct node));
	new_node->id.type = type;
	new_node->time = time;
	new_node->next = NULL;
	if (pq->head == NULL) { // no elements in list
		pq->head = new_node;
		pq->tail = new_node;
		pq->length = 1;
	}
	else if (time < pq->head->time) { // new element in head of pq
		new_node->next = pq->head;
		pq->head = new_node;
		pq->length++;
	}
	else { // new element is somewhere else
		struct node *current_node = pq->head;
		while (current_node->next != NULL && time > current_node->next->time) {
			current_node = current_node->next;
		}
		// current node is now the node before where the new node should go
		new_node->next = current_node->next;
		current_node->next = new_node;
		pq->length++;
		if (new_node->next == NULL)
			pq->tail = new_node;
	}
}

struct node pop(struct linked_list *q) {
	struct node popped = *(q->head);
	if (q->head == q->tail) {
		free(q->head);
		q->head = q->tail = NULL;
	}
	else {
		struct node *next = q->head->next;
		free(q->head);
		q->head = next;
	}
	q->length--;
	return popped;
}

//Karl Schaller 915580340

void main(int argc, char **argv, char** envp) {

	if (argc > 1) {
		int infile = open(argv[1], O_RDONLY);
		dup2(infile, STDIN_FILENO);
	}
	
	while (1) {
		
		print("myshell> ");
		
		size_t linesize = 32;
		char *line = (char *)malloc(linesize * sizeof(char));
		getline(&line, &linesize, stdin);
		
		if (strcmp(line, "exit") == 0 || strcmp(line, "^D") == 0)
			exit(0);
			
		// TODO Parse input into arguments
		char *argv2[linesize];
		argv2[0] = strtok(line, " ");
		int argc2 = 1;
		for (; argv2[argc2] != NULL; argc2++);
			argv2[argc2] = strtok(NULL, " ");
			
			
		char *command = argv2[0];
		
		
		// Built in commands
		if (strcmp(command, "cd") == 0)
			cd(argc2, argv2);
		else if (strcmp(command, "clr") == 0)
			clr();
		else if (strcmp(command, "dir") == 0)
			dir(argc2, argv2);
		else if (strcmp(command, "environ") == 0)
			environ(envp);
		else if (strcmp(command, "echo") == 0)
			echo();
		else if (strcmp(command, "help") == 0)
			help();
		else if (strcmp(command, "pause") == 0)
			pause();
		
		
		// External commands
		else if (fork() == 0) {	// run external command
			// TODO
			if ((int i = argsearch(argv2, "|")) != -1) {
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
					argv2 = argv2[i+1]; // does this work?
				}
			}
			if ((int i = argsearch(argv2, "<")) != -1) {
				int infile = open(argv2[i+1], read)
				dup2(infile, STDIN_FILENO)
			}
			if ((int i = argsearch(argv2, ">")) != -1) {
				int outfile = open(argv2[i+1], trunc)
				dup2(outfile, STDOUT_FILENO)
			}
			else if ((int i = argsearch(argv2, ">>")) != -1) {
				int outfile = open(argv2[i+1], append)
				dup2(outfile, STDOUT_FILENO)
			}
			execute(argc, argv)
		}
		else { // wait for external command to finish
			// TODO
			if (argsearch(argv2, "&") == -1)
				int status = 0;
				wait(&status);
				cout << "Child exited with status of " << status << endl
		}
		
		
		free(line);
	}
}

// Change the directory
void cd(int argc, char **argv) {
	// TODO
	if (argv2 > 1 and argv2[1] != redirect symbols) {
		if chdir(argv[1]) != 0
			error
	}
	else
		chdir(getenv("HOME"));
}
	
// Clear the screen
void clr() {
	printf("\33[H\033[2J");
}
	
// Print the contents of the current directory or a user-specified directory
void dir(int argc, char **argv) {
	// TODO
	if argv2 > 1 and argv2[1] != redirect symbols
		DIR *dir = opendir(argv[1]);
	else
		DIR *dir = opendir("./");
	struct dirent *s;
	while ((s = readdir(dir)) != NULL)
		print("%s\t", s->d_name);
}

// List the environment variables
void environ(char **envp) {
	for (i = 0; envp[i] != NULL; i++)
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
void pause() {
	while (getchar() != 'n\')
		fflush(stdin);
}

// Consider input to be program invocation
void execute(int argc, char **argv) {
	// TODO
	int fd = open(argv[0]);
	if (fd != -1) {
		execvp(argv[0], argv);
		return;
	}
	
	char *path = getenv("PATH");
	// TODO
	char *paths[100] = parse path by ':' and terminate with NULL
	for dir in paths {
		fd = open(dir + argv[0]);
		if (fd != -1) {
			execvp(dir + argv[0], argv);
			return;
		}
	}
	
	print(file not found)
}

// Search argv for key and return index or -1 if not found
int argsearch(char **argv, char *key) {
	for (int i = 0; argv[i] != NULL; i++) {
		if (strcmp(argv[i], key) == 0)
			return i;
	}
	return -1;
}