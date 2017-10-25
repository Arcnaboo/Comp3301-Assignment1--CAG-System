/*
 * @course: Comp 3301
 * @task: Assignment 1
 * @author: Arda 'ARC' Akgur
 * @file: s4382911_usershell.c
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

// To move the cursor around and clear Terminal
#define clear_terminal() printf("\033[H\033[J")
#define move_cursor(x,y) printf("\033[%d;%dH", (x), (y))

// Color codes for terminal print
#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

// Thread Function Pointers
void *process_updater(void *masterPointer);
void *waiter(void *pidPointer);

// Pointer to display pipe file for signal handler
FILE* writeFileCopy;

// Master Type, stored everything required
typedef struct {
    
    pid_t *pids;
    int processAmount;
    int pipes[1000][2];
    FILE* files[1000];
    
    int readPipe[2];
    int writePipe[2];
    FILE* readFile;
    FILE* writeFile;
    
    pthread_t processUpdater;
    bool threadRun; 
    
} Master;

// Function Dictionary
typedef struct {
    
    void (*val[11]) (Master *master, char* command);
    char** key;
} FunctionMap;

// Function prototypes for the FunctionMap
void process_cell(Master *master, char* command);
void process_still(Master *master, char* command);
void process_osc(Master *master, char* command);
void process_ship(Master *master, char* command);
void process_start(Master *master, char* command);
void process_stop(Master *master, char* command);
void process_clear(Master *master, char* command);
void process_help(Master *master, char* command);
void process_end(Master *master, char* command);
void process_arc(Master *master, char* command);
void process_pause(Master *master, char* command);

// Error types
typedef enum {
    
    OK = 0,
    INVALID = 1,
    PIPE = 2,
    ARGUMENT = 3,
    SYSERR = 20
} Erre;

// Prints error and extits and returns OK if only INVALID command
Erre show_err(Erre e) {
    
    char* errstring = malloc(sizeof(char*) * 100);
    
    if (e == INVALID) {
        
		printf("%sInvalid command: ", YELLOW);
		return OK;
	}
    
    switch (e) {
        
        case OK: 
            break;
            
        case PIPE:
            errstring = "Unexpected loss of child process\n";
            break;
            
        case ARGUMENT:
			errstring = "Too many arguments, Usage: ./a1shell\n";
			break;
            
        case SYSERR:
			errstring = "System error\n";
			break;
            
		case INVALID: 
			break;			
    }
    ;
    fprintf(stderr, "%s", errstring);
    exit(e);
}

// Arranges the commands to the FunctionMap
void arrange_commands(FunctionMap *map) {
	
	map->key = malloc(sizeof(char*) * 11);

	for (int i = 0; i < 11; i++) {
        
		map->key[i] = malloc(sizeof(char) * 6);
	}
    
	strcat(map->key[0], "cell");
	strcat(map->key[1], "still");
	strcat(map->key[2], "osc");
	strcat(map->key[3], "ship");
	strcat(map->key[4], "start");
	strcat(map->key[5], "stop");
	strcat(map->key[6], "clear");
	strcat(map->key[7], "help");
	strcat(map->key[8], "end");
    strcat(map->key[9], "arc");
    strcat(map->key[10], "pause");
    
	map->val[0] = process_cell;
	map->val[1] = process_still;
	map->val[2] = process_osc;
	map->val[3] = process_ship;
	map->val[4] = process_start;
	map->val[5] = process_stop;
	map->val[6] = process_clear;
	map->val[7] = process_help;
	map->val[8] = process_end;
    map->val[9] = process_arc;
    map->val[10] = process_pause;
}

// Checks the command
// if valid command then returns true
bool check_command(char* command) {
    
	char* check = malloc(sizeof(char) * 6);
	for (int i = 0; i < strlen(command); i++) {
        
		if (command[i] == ' ' || command[i] =='\n') {
            
			check[i] = '\0';
			break;
		}
		check[i] = command[i];
	}
	if (strcmp(check, "cell") == 0 || strcmp(check, "still") == 0 ||
		strcmp(check, "osc") == 0 || strcmp(check, "ship") == 0 ||
		strcmp(check, "start") == 0 || strcmp(check, "stop") == 0 ||
		strcmp(check, "clear") == 0 || strcmp(check, "help") == 0 ||
		strcmp(check, "end") == 0 || strcmp(check, "arc") == 0 ||
        strcmp(check, "pause") == 0) {

		free(check);
		return true;
	}
	free(check);
	return false;
	
}

// Processes the given command using FunctionMap
void process_command(Master *master, FunctionMap *map, char* command) {
    
	command[strlen(command) - 1] = '\0';
    char temp[strlen(command)];
    
    for (int i = 0; i < strlen(command); i++) {
        
        if (command[i] == ' ') {
            
            temp[i] = '\0';
            break;
        }
        temp[i] = command[i];
    }
    
	for (int i = 0; i < 11; i++) {
        
		if (strcmp(temp, map->key[i]) == 0) {
            
			map->val[i](master, command);
			break;
		}
        
        if (strcmp(command, map->key[i]) == 0) {
            
			map->val[i](master, command);
			break;
		}
	}
}

// Prints the Default Output
void print_default(void) {
    
    clear_terminal();
    move_cursor(0, 0);
    printf("%sWelcome to COMP3301 A1 User Shell\n", CYAN);
}

// Executes the CAG program
void run_cag(Master *master) {
    
    close(master->writePipe[1]);
    close(master->readPipe[0]);
    
    dup2(master->writePipe[0], 0);
    dup2(master->readPipe[1], 1);
    
    char** args = malloc(sizeof(char*) * 2);
    args[0] = "./cag";
    args[1] = (char*) 0;
    
    if (execvp(args[0], args) == -1) {
        
        fprintf(stderr, "Exec erroor\n");
        exit(21);
    }
}

// Signal Handler
static void ctrl_c_handler(int sig) {
    
    printf("Terminating CAG\n");
	fprintf(writeFileCopy, "end\n");
    fflush(writeFileCopy);
    exit(0);
}

// main Loop
int main(int argc, char** argv) {
    
    Master master;
    struct sigaction usr1Act;
    
	memset(&usr1Act, 0, sizeof(struct sigaction));
	usr1Act.sa_handler = ctrl_c_handler;

	usr1Act.sa_flags = SA_RESTART;

	sigaction(SIGINT, &usr1Act, NULL);

    master.processAmount = 0;
    master.threadRun = true;
    
    pipe(master.readPipe);
    pipe(master.writePipe);
    
    master.writeFile = fdopen(master.writePipe[1], "w");
    writeFileCopy = master.writeFile;
    
    master.readFile = fdopen(master.readPipe[0], "r");
    
    int pid = fork();
    
    if (pid == 0) {
        run_cag(&master);
    }
    
	FunctionMap map;
	arrange_commands(&map);
    
    print_default();
    
    if (pthread_create(&master.processUpdater, NULL, process_updater, &master)) {
        
            fprintf(stderr, "Error creating thread\n");
            exit(20);
    }
    
    char buffer[255];
    
	while (true) {
        
		printf("%s:^) %s", RED, GREEN);
        fgets(buffer, 255, stdin);
        
		if (!check_command(buffer)) {
            
			show_err(INVALID);
			printf("%sPlease write * help * for available commands\n", buffer);
			continue;
		}
        
		process_command(&master, &map, buffer);
	} 
	
	return 0;
}

// Command Runner Function
// fork() and runs the command
// exits if receives mark
// receives messages from processUpdater thread of the main process 
// stops its own processUpdater thread
// also creates a new thread waiter
// waiter waits for this child to end
int command_runner(Master *master, char* command, int index) {

    int pid = fork();
    
    if (pid == 0) {
        
        if (pthread_join(master->processUpdater, NULL)) {

            fprintf(stderr, "Error joining thread\n");
            exit(2);
        }
        
        fprintf(master->writeFile,"%s\n", command);
        fflush(master->writeFile);
        
        close(master->pipes[index][1]);
        printf("Child started pid %d\n", getpid());
        
        unsigned int start = 0;
        
        FILE* file = fdopen(master->pipes[index][0], "r");
        
        while (true) {
            
            char* buffer = malloc(sizeof(char) * 100);
            fgets(buffer, 100, file);
            
            char* ptr;
            int myId = strtol(buffer, &ptr, 10);
          
            if (buffer[1] == 'd') {
                
                printf("Organism %d died - total alive time: %u seconds\n", myId, start);
                printf("Exiting child\n");
                exit(0);
            }
            
            if (buffer[1] == 'p') {
                
                while(true) {
                    
                    fgets(buffer, 100, file);
                    if (buffer[1] == 'p') {
                        
                        break;
                    }
                    sleep(1);
                    start++;
                }
            }
            sleep(1);
            start++;
            
            if (buffer[1] == 'a') {
                
                printf("Organism %d alive for %u seconds\n", myId, start);
            }
            if (buffer[1] == 'e') {
                
                printf("Organism %d evolved and alive for %u seconds\n", myId, start);
            }
        }
	} else {
        
        pthread_t waitThread;
        
        if (pthread_create(&waitThread, NULL, waiter, &pid)) {
            
            fprintf(stderr, "Error creating thread\n");
            exit(20);
        }
        return 1;
    }       
}

// Processes cell command
void process_cell(Master *master, char* command) {
    
    if (command[5] == 'd') {

        fprintf(master->writeFile, "%s\n", command);
        fflush(master->writeFile);
        return;
    }
    
    pipe(master->pipes[master->processAmount]);
    master->files[master->processAmount] = fdopen(master->pipes[master->processAmount][1], "w");
    
    if (command_runner(master, command, master->processAmount)) {
        
        master->processAmount++;
    }
}

// Processes still command
void process_still(Master *master, char* command) {
    
	pipe(master->pipes[master->processAmount]);
    master->files[master->processAmount] = fdopen(master->pipes[master->processAmount][1], "w");
    
    if (command_runner(master, command, master->processAmount)) {
        
        master->processAmount++;
    }
}

// Processes osc command
void process_osc(Master *master, char* command) {
    
	pipe(master->pipes[master->processAmount]);
    master->files[master->processAmount] = fdopen(master->pipes[master->processAmount][1], "w");
    
    if (command_runner(master, command, master->processAmount)) {
        
        master->processAmount++;
    }
}

// Processes ship command
void process_ship(Master *master, char* command) {
    
	pipe(master->pipes[master->processAmount]);
    master->files[master->processAmount] = fdopen(master->pipes[master->processAmount][1], "w");
    
    if (command_runner(master, command, master->processAmount)) {
        
        master->processAmount++;
    }
}

// Processes pause command
void process_pause(Master *master, char* command) {
    
    for (int i = 0; i < master->processAmount; i++) {
        
        fprintf(master->files[i], "%dpause\n", i);
        fflush(master->files[i]);
    }
}

// Processes start command
void process_start(Master *master, char* command) {
    
	fprintf(master->writeFile, "start\n");
    fflush(master->writeFile);
}

// Prcesses stop command
void process_stop(Master *master, char* command) {
    
	fprintf(master->writeFile, "stop\n");
    fflush(master->writeFile);
}

// Processes clear command
void process_clear(Master *master, char* command) {
    
	fprintf(master->writeFile, "clear\n");
    fflush(master->writeFile);
}
// processes help command
void process_help(Master *master, char* command) {
    
	printf("%sCOMP3301 Cellular Automaton Help\n", CYAN);
	printf("%sCommand %sParam %sDescr\n", GREEN, YELLOW, WHITE);
	printf("%s======= %s======%s ======\n", GREEN, YELLOW, WHITE);
	
	printf("%scell %s<state> <x> <y>", GREEN, YELLOW);
	printf("%s Draw a cell at x and y coordinates.", WHITE);
	printf("%s State: %s‘alive’ %sor %s‘dead’\n", CYAN, GREEN, WHITE, RED);
	
	printf("%sstill %s<type> <x> <y>", GREEN, YELLOW);
	printf("%s Draw a still life at x and y coordinates.", WHITE);
	printf("%s Note x and y are at the top left edge of the figure. ", WHITE);
	printf("%s Types: %sblock, %sbeehive, %sloaf, %sboat.\n", CYAN, GREEN, MAGENTA, WHITE, RED);
	
	printf("%sosc %s<type> <x> <y>", GREEN, YELLOW);
	printf("%s Draw an oscillator at x and y coordinates.", WHITE);
	printf("%s Note x and y are at the top left edge of the figure. ", WHITE);
	printf("%s Types: %sblinker, %stoad %sand %sbeacon.\n", CYAN, GREEN, MAGENTA, WHITE, RED);
	
	printf("%sship %s<type> <x> <y>", GREEN, YELLOW);
	printf("%s Draw a spaceship at x and y coordinates.", WHITE);
	printf("%s Note x and y are at the top left edge of the figure. ", WHITE);
	printf("%s Types: %sglider.\n", CYAN, GREEN);
	
	printf("%sstart %s=====", GREEN, YELLOW);
	printf("%s Start or restart all cellular automation\n", WHITE);
	
	printf("%sstop %s=====", GREEN, YELLOW);
	printf("%s Stop all cellular automation\n", WHITE);
	
	printf("%sclear %s=====", GREEN, YELLOW);
	printf("%s Clear all visual cellular automation\n", WHITE);
	
	printf("%shelp %s=====", GREEN, YELLOW);
	printf("%s Displays list and summary of each command.\n", WHITE);
	
	printf("%send %s=====", GREEN, YELLOW);
	printf("%s End all processes and threads associated with the CAG.\n", WHITE);
}

// processes end command
void process_end(Master *master, char* command) {
	fprintf(master->writeFile, "end\n");
    fflush(master->writeFile);
}

// processes arc command 
void process_arc(Master *master, char* command) {
	
}

// Process updater thread function
void *process_updater(void *masterPointer) {
    
    Master *master = (Master *)masterPointer;
    
    while (true) {

        char *message = malloc(sizeof(char) * 100);
        fgets(message, 100, master->readFile);
        
        char* ptr;
        
        int id = strtol(message, &ptr, 10);
        
        fprintf(master->files[id], "%s", message);
        fflush(master->files[id]);
        
        free(message);
                   
    }
    return NULL;
}

// Process waiter thread function
void *waiter(void *pidPointer) {
    
    int *pids = (int *)pidPointer;
    int pid = *pids;
    int got_pid, status;

	/* Wait until child process has finished */
	while ((got_pid = wait(&status))) {   /* go to sleep until something happens */

		/* wait woke up */
		if (got_pid == pid) {
            
            break;	/* this is what we were looking for */
        }
      
		if ((got_pid == -1) && (errno != EINTR)) {
            
			/* an error other than an interrupted system call */
			perror("waitpid");
			exit(30);
		}

	}

	if (WIFEXITED(status))	/* process exited normally */
		printf("child process exited with value %d\n", WEXITSTATUS(status));

	else if (WIFSIGNALED(status))	/* child exited on a signal */
		printf("child process exited due to signal %d\n", WTERMSIG(status));

	else if (WIFSTOPPED(status))	/* child was stopped */
		printf("child process was stopped by signal %d\n", WIFSTOPPED(status));
        
    while (true) {
        ;
    }
    return NULL;
}