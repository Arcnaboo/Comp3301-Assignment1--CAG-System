/*
 * @course: Comp 3301
 * @task: Assignment 1
 * @author: Arda 'ARC' Akgur
 * @file: s4382911_cag.c
 * gcc -Wall -lpthread -levent -std=gnu99 cag.c -o cag
 * */


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

// colors for print
#define NORMAL  "\x1B[0m"
#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE  "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN  "\x1B[36m"
#define WHITE  "\x1B[37m"

// height and width of the game
unsigned int height = 20;
unsigned int width = 20;

// threadid tracker, threads takes it and increments it
unsigned int threadId = 0;

// helps finding adjacent cells
int neighborHelper[16] = {0, 1, 0, -1, 1, 0, -1, 0, -1, -1, -1, 1, 1, -1, 1, 1};

// Thread function prototypes
void *game_control(void *masterPointer);
void *row_control(void *masterPointer);
void *display_control(void *masterPointer);
void *organism_control(void *masterPointer);

// Counts how many word in this command
int count_command_elements(char *command) {
    
    int count = 1;
    for (int i = 0; i < strlen(command); i++) {
        
        if (command[i] == ' ') {
            
            count++;
        }
    }
    return count;
}

// Type to define a point in the grid
typedef struct {
    
    int row;
    int col;
    bool alive;
} Node;

// Type to define Organisms,
// multiple alive cells making 1 organism
typedef struct {
    
    Node topLeftNode; // top left corner
    Node botRightNode; // bot right corner
    Node nodes[400]; // each cell is a node
    Node originalNodes[400];
    int length; // length of nodes
    int originalLength;
    bool alive; // is organism alive, at least 1 alive cell
    char state; // current highest state '1'..'5'
    int type; // 0 single cell, 1 still, 2 osc, 3 ship
    int organismId; // organism id 
    bool evolved;
} Organism;

// Master game struct
typedef struct {
    
    char **grid; // game grid
    char **temp; // temporary grid
    bool **tickBox; // each row thread ticks elements of the array corresponds to current turn
    unsigned int turn; // current turn
    bool turnComplete; // is current turn complete
    pthread_t *rows; // all the row threads
    unsigned int *threadIds; // all the thread ids
    bool lock; // true false lock
    bool *goAhead; // element is true if corresponding turn is ready to play
    bool gameOn; // is cellular autamation active
    int displayPipe[2]; // IPC with display
    FILE* displayFile; // IPC file
    Organism *organisms; // all the alive organisms
    int organismAmount; // amount of unique organism
} Master;

// FunctionMap dictionary
typedef struct {
    
    void (*val[8]) (Master *master, char *command);
    char **key;
} FunctionMap;

// returns true if position in the grid
bool is_position_possible(int x, int y) {
    
    if (x >= height || x < 0 || y >= width || y < 0) {
        
        return false;
    }
    return true;
}

// Creates an organism and updates the amount of organism in Master
void create_organism(Master *master, int x, int y, int *initialOrganism, int length, int type) {
    
    Organism organism;
    int orgLength = 0;
    
    for (int i = 0; i < length; i += 2) {
        
        if (is_position_possible(x + initialOrganism[i], y + initialOrganism[i + 1])) {
            
            organism.originalNodes[orgLength].row = x + initialOrganism[i];
            organism.originalNodes[orgLength].col = y + initialOrganism[i + 1];
            organism.originalNodes[orgLength].alive = true;
            
            organism.nodes[orgLength].row = x + initialOrganism[i];
            organism.nodes[orgLength].col = y + initialOrganism[i + 1];
            organism.nodes[orgLength++].alive = true;   
        }
    }
    
    organism.originalLength = orgLength;
    
    organism.topLeftNode = organism.nodes[0];
    
    organism.botRightNode = organism.nodes[orgLength - 1];
    
    organism.alive = true;
    
    organism.type = type;
    
    organism.length = orgLength;
    
    organism.state = '1';
    
    organism.evolved = false;
    
    organism.organismId = master->organismAmount;
    
    master->organisms[master->organismAmount++] = organism; 
}

// processes ship command 
void process_ship_command(Master *master, char* command) {
   
   if (count_command_elements(command) < 4) {
       
        printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        return;
    }
    
    const char delim[2] = " ";
    char *token, *type, *ptr;
    int x, y;
    
    token = strtok(command, delim);
    token = strtok(NULL, delim);
    
    type = strdup(token);
    
    token = strtok(NULL, delim);
    
    x = strtol(token, &ptr, 10);
    
    token = strtok(NULL, delim);
    
    y = strtol(token, &ptr, 10);
    
    int glider[10] = {0, 1, 1, 2, 2, 0, 2, 1, 2, 2};
    
    while (true) {
        
        if (!master->lock) {
            
            master->lock = true;
        } else {
            continue;
        }
        
        if (x < 0 || x >= height || y < 0 || y > width) {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        if (strcmp(type, "glider") == 0) {
            
            create_organism(master, x, y, glider, 10, 0);
            
            for (int i = 0; i < 10; i += 2) {
                
                if (is_position_possible(x + glider[i], y + glider[i + 1])) {
                    
                    master->grid[x + glider[i]][y + glider[i + 1]] = '1';
                    master->temp[x + glider[i]][y + glider[i + 1]] = '1';
                }
            }
        } else {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        
        printf("%dconfirm\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        
        master->lock = false;
        
        break;
   }
}

// Processes osc command
void process_osc_command(Master *master, char* command) {
    
    if (count_command_elements(command) < 4) {
        
        printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        return;
    }
    
    const char delim[2] = " ";
    char *token, *type, *ptr;
    int x, y;
    
    token = strtok(command, delim);
    token = strtok(NULL, delim);
    
    type = strdup(token);
    
    token = strtok(NULL, delim);
    
    x = strtol(token, &ptr, 10);
    
    token = strtok(NULL, delim);
    
    y = strtol(token, &ptr, 10);
    
    int blinker[6] = {0, 0, -1, 0, 1, 0};
    int toad[12] = {0, 0, 0, 1, 1, -1, 3, 0, 3, 1, 2, 2};
    int beacon[16] = {0, 0, 0, 1, 1, 0, 1, 1, 2, 2, 2, 3, 3, 2, 3, 3};
                    
    while (true) {
        
        if (!master->lock) {
            
            master->lock = true;
        } else {
            continue;
        }
        if (x < 0 || x >= height || y < 0 || y > width) {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        
        if (strcmp(type, "blinker") == 0) {
            
            create_organism(master, x, y, blinker, 6, 2);
            
            for (int i = 0; i < 6; i += 2) {
                
                if (is_position_possible(x + blinker[i], y + blinker[i + 1])) {
                    
                    master->grid[x + blinker[i]][y + blinker[i + 1]] = '1';
                    master->temp[x + blinker[i]][y + blinker[i + 1]] = '1';
                }
            }
        } else if (strcmp(type, "toad") == 0) {
            
            create_organism(master, x, y, toad, 12, 2);
            
            for (int i = 0; i < 12; i += 2) {
                
                if (is_position_possible(x + toad[i], y + toad[i + 1])) {
                    
                    master->grid[x + toad[i]][y + toad[i + 1]] = '1';
                    master->temp[x + toad[i]][y + toad[i + 1]] = '1';
                }
            }
        } else if (strcmp(type, "beacon") == 0) {
            
            create_organism(master, x, y, beacon, 16, 2);
            
            for (int i = 0; i < 16; i += 2) {
                
                if (is_position_possible(x + beacon[i], y + beacon[i + 1])) {
                    
                    master->grid[x + beacon[i]][y + beacon[i + 1]] = '1';
                    master->temp[x + beacon[i]][y + beacon[i + 1]] = '1';
                }
            }
        } else {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        
        printf("%dconfirm\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        
        master->lock = false;
        break;
   }
}

// Processes still command 
void process_still_command(Master *master, char* command) {
    
    if (count_command_elements(command) < 4) {
        
        printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        return;
    }
    
    const char delim[2] = " ";
    char *token, *type, *ptr;
    int x, y;
    
    token = strtok(command, delim);
    token = strtok(NULL, delim);
    
    type = strdup(token);
    
    token = strtok(NULL, delim);
    
    x = strtol(token, &ptr, 10);
    
    token = strtok(NULL, delim);
    
    y = strtol(token, &ptr, 10);
    
    int block[8] = {0, 0, 0, 1, 1, 0, 1, 1};
    int beehive[12] = {0, 0, 0, 1, 2, 0, 2, 1, 1, -1, 1, 2};
    int loaf[14] = {0, 0, 1, -1, 1, 1, 2, 1, 3, 0, 3, -1, 2, -2};
    int boat[10] = {0, 0, 0, 1, 1, 0, 2, 1, 1, 2};
    
    while (true) {
        
        if (!master->lock) {
            
            master->lock = true;
        } else {
            continue;
        }
        if (x < 0 || x >= height || y < 0 || y > width) {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        if (strcmp(type, "block") == 0) {
            
            create_organism(master, x, y, block, 8, 1);
            
            for (int i = 0; i < 8; i += 2) {
                
                if (is_position_possible(x + block[i], y + block[i + 1])) {
                    
                    master->grid[x + block[i]][y + block[i + 1]] = '1';
                    master->temp[x + block[i]][y + block[i + 1]] = '1';
                }
            }
        } else if (strcmp(type, "beehive") == 0) {
            
            create_organism(master, x, y, beehive, 12, 1);
            
            for (int i = 0; i < 12; i+= 2) {
                
                if (is_position_possible(x + beehive[i], y + beehive[i + 1])) {
                    
                    master->grid[x + beehive[i]][y + beehive[i + 1]] = '1';
                    master->temp[x + beehive[i]][y + beehive[i + 1]] = '1';
                }
            }
        } else if (strcmp(type, "loaf") == 0) {
            
            create_organism(master, x, y, loaf, 14, 1);
            
            for (int i = 0; i < 14; i += 2) {
                
                if (is_position_possible(x + loaf[i], y + loaf[i + 1])) {
                    
                    master->grid[x + loaf[i]][y + loaf[i + 1]] = '1';
                    master->temp[x + loaf[i]][y + loaf[i + 1]] = '1';
                }
            }
        } else if (strcmp(type, "boat") == 0) {
            
            create_organism(master, x, y, boat, 10, 1);
            
            for (int i = 0; i < 10; i += 2) {
                
                if (is_position_possible(x + boat[i], y + boat[i + 1])) {
                    
                    master->grid[x + boat[i]][y + boat[i + 1]] = '1';
                    master->temp[x + boat[i]][y + boat[i + 1]] = '1';
                }
            }
        } else {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        printf("%dconfirm\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        
        master->lock = false;
        break;
   }
    
}

// processes cell command
void process_cell_command(Master *master, char *command) {
    
    if (count_command_elements(command) < 4) {
        
        printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        return;
    }
    
    const char delim[2] = " ";
    char *token, *state, *ptr;
    int x, y;
    
    token = strtok(command, delim);
    token = strtok(NULL, delim);
    
    state = strdup(token);
    
    token = strtok(NULL, delim);
    
    x = strtol(token, &ptr, 10);
    
    token = strtok(NULL, delim);
    
    y = strtol(token, &ptr, 10);
   
    while (true) {
        
        if (!master->lock) {
            
            master->lock = true;
        } else {
            continue;
        }
        if (x < 0 || x >= height || y < 0 || y > width) {
            
            printf("%dinvalid\n", master->organisms[master->organismAmount - 1].organismId);
            fflush(stdout);
            
            master->lock = false;
            return;
        }
        if (strcmp(state, "alive") == 0) {
            
            int positions[2];
            positions[0] = x;
            positions[1] = y;
            
            create_organism(master, x, y, positions, 2, -1);
            
            master->grid[x][y] = '1';
            
        } else {
            
            master->grid[x][y] = '0';
            
            for (int i = 0; i < master->organismAmount; i++) {
                
                if (master->organisms[i].type == -1 && master->organisms[i].topLeftNode.row == x 
                        && master->organisms[i].topLeftNode.col == y) {
                            
                    master->organisms[i].alive = false;
                    printf("%dconfirm\n", master->organisms[i].organismId);
                    break;
                }
            }
            
            master->lock = false;
            return;
        }
        
        printf("%dconfirm\n", master->organisms[master->organismAmount - 1].organismId);
        fflush(stdout);
        
        master->lock = false;
        break;
   }
}

// processes end command 
void process_end_command(Master *master, char* command) {
    
    fprintf(master->displayFile,"end\n");
    fflush(master->displayFile);
    printf("Terminating CAG\n");
    fflush(stdout);
    exit(0);
}

// processes clear command 
void process_clear_command(Master *master, char* command) {
    
    while (true) {
        if (!master->lock) {
            
            master->lock = true;
        } else {
            continue;
        }
        
        for (int i = 0; i < height; i++) {
            
            memset(master->grid[i], '0', width);
            memset(master->temp[i], '0', width);
        }
        for (int i = 0; i < master->organismAmount; i++) {
            
            master->organisms[i].alive = false;
        }
        
        master->lock = false;
        break;
    }
    
}

// processes start command 
void process_start_command(Master *master, char* command) {
    
    master->gameOn = true;
}

// pricesses stop command 
void process_stop_command(Master *master, char* command) {
    
    master->gameOn = false;
}

// Arranges commands for dictionary 
void arrange_commands(FunctionMap *map) {
	
	map->key = malloc(sizeof(char*) * 8);
    
	for (int i = 0; i < 8; i++) {
        
		map->key[i] = malloc(sizeof(char) * 6);
	}
	strcat(map->key[0], "cell");
	strcat(map->key[1], "still");
	strcat(map->key[2], "osc");
	strcat(map->key[3], "ship");
	strcat(map->key[4], "start");
	strcat(map->key[5], "stop");
	strcat(map->key[6], "clear");
	strcat(map->key[7], "end");
    
	map->val[0] = process_cell_command;
	map->val[1] = process_still_command;
	map->val[2] = process_osc_command;
	map->val[3] = process_ship_command;
	map->val[4] = process_start_command;
	map->val[5] = process_stop_command;
	map->val[6] = process_clear_command;
	map->val[7] = process_end_command;
}

// Checks the validity of the command 
bool check_command(char* command, char* check) {

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
		strcmp(check, "end") == 0) {
            
		return true;
	}
	return false;
}

// Processes the given command 
void process_command(Master *master, FunctionMap *map, char *command, char *check) {
    
	command[strlen(command) - 1] = '\0';
    
	for (int i = 0; i < 8; i++) {
		if (strcmp(check, map->key[i]) == 0) {
			map->val[i](master, command);
			break;
		}
	}
}

// Allocated the CAG 
void allocate_cag(Master *master) {
    
    master->grid = malloc(sizeof(char *) * height);
    master->temp = malloc(sizeof(char *) * height);
    master->rows = malloc(sizeof(pthread_t) * height);
    master->threadIds = malloc(sizeof(unsigned int) * height);
    
    master->tickBox = malloc(sizeof(bool *) * 5000);
    master->goAhead = malloc(sizeof(bool) * 5000);
    master->organisms = malloc(sizeof(Organism) * 5000);
    
    for (int i = 0; i < height; i++) {
        
        master->grid[i] = malloc(sizeof(char) * width + 1);
        master->temp[i] = malloc(sizeof(char) * width + 1);
        
        memset(master->grid[i], '0', width);
        memset(master->temp[i], '0', width);
        
        master->grid[i][width]='\0';
        master->temp[i][width]='\0';
    }
    
    master->organismAmount = 0;
    master->turn = 0;
    
    master->gameOn = false;
    master->turnComplete = false;
    master->lock = false;
    
    for (int i = 0; i < 5000; i++) {
        
        master->tickBox[i] = malloc(sizeof(bool) * height);
        master->goAhead[i] = false;
    }
}

// Prints the grid if needed to stdout
void print_grid(Master *master) {
    
    for (int i = 0; i < height; i++) {
        
        for (int j = 0; j < width; j++) {
            
            if (master->grid[i][j] == '0') {
                
                printf("%s%c", NORMAL, master->grid[i][j]);
                
            } else if (master->grid[i][j] == '1') {
                
                printf("%s%c", RED, master->grid[i][j]);
                
            } else if (master->grid[i][j] == '2') {
                
                printf("%s%c", MAGENTA, master->grid[i][j]);
                
            } else if (master->grid[i][j] == '3') {
                
                printf("%s%c", GREEN, master->grid[i][j]);
                
            } else if (master->grid[i][j] == '4') {
                
                printf("%s%c", CYAN, master->grid[i][j]);
                
            } else if (master->grid[i][j] == '5') {
                
                printf("%s%c", YELLOW, master->grid[i][j]);
                
            }
            if (j == width - 1) {
                printf("%s%c", NORMAL, '\n');
            }
        }
    }
}

// Prints the grid to the pipe
void print_grid_to_pipe(Master *master) {
    
    for (int i = 0; i < height; i++) {
        
        for (int j = 0; j < width; j++) {
            
            if (j == 0) {
                
                fprintf(master->displayFile, "[%d,%c,", i, master->grid[i][j]);
                
            } else if (j == width - 1) {
                
                fprintf(master->displayFile, "%c]\n", master->grid[i][j]);
                fflush(master->displayFile);
                
            } else {
                
                fprintf(master->displayFile, "%c,", master->grid[i][j]);
            }
        }
    }
    
}

// Executes the xLib displayer program
void handle_child(Master *master) {
    
    close(master->displayPipe[1]);
    dup2(master->displayPipe[0], 0);
    
    int devNull = open("/dev/null", 2, "w");
    dup2(devNull, 2);
    
    //dup2(devNull, 1);
    char** args = malloc(sizeof(char*) * 2);
    args[0] = "./cagdisplay";
    args[1] = (char*) 0;
    
    if (execvp(args[0], args) == -1) {
        
        fprintf(stderr, "Exec erroor\n");
        exit(21);
    }
}

// Singal handler 
static void ctrl_c_handler(int sig) {
    
    printf("Terminating CAG\n");
	exit(0);
}

// Main loop
int main(int argc, char** argv) {
    
    if (argc != 1) {
        
        fprintf(stderr,"Usage: %s\n", argv[0]);
        exit(1);
    }
    
    struct sigaction usr1Act;
	memset(&usr1Act, 0, sizeof(struct sigaction));
	usr1Act.sa_handler = ctrl_c_handler;
	usr1Act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &usr1Act, NULL);
    
    Master master;
    allocate_cag(&master);
    
    pipe(master.displayPipe);
    master.displayFile = fdopen(master.displayPipe[1], "w");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        
        handle_child(&master);
        printf("hmmm\n");
    }
    
    FunctionMap map;
    arrange_commands(&map);
    
    pthread_t gameThread, displayThread, organismThread;

    if (pthread_create(&gameThread, NULL, game_control, &master)) {
        
            fprintf(stderr, "Error creating thread\n");
            exit(20);
    }
    
    if (pthread_create(&displayThread, NULL, display_control, &master)) {
        
        fprintf(stderr, "Error creating thread\n");
        exit(20);
    }
    
    if (pthread_create(&organismThread, NULL, organism_control, &master)) {
        
        fprintf(stderr, "Error creating thread\n");
        exit(20);
    }
    
    for (int i = 0; i < height; i++) {
        
        if(pthread_create(&master.rows[i], NULL, row_control, &master)) {
            
            fprintf(stderr, "Error creating thread\n");
            exit(20);
        }
    }
     
    while (true) {
        
        char buffer[255];

        if (!fgets(buffer, 255, stdin)) {
            
            fprintf(stderr, "Error reading input\n");
            exit(6);
        }
        
        char *check = malloc(sizeof(char) * 6);
        
        if (!check_command(buffer, check)) {
            
			printf("invalid\n");
            fflush(stdout);
			continue;
		}
		process_command(&master, &map, buffer, check);
        //print_grid_to_pipe(&master);
    }    
    return 0;
}

// Assigns id value to thread
unsigned int get_id(Master *master) {
    
    unsigned int result;
    
    while(true) {
        if (master->lock) {
            
            continue;
            
        } else {
            
            master->lock = true;
            
            result = threadId;
            
            threadId++;
            
            master->lock = false;
            
            break;
        }
    }
    return result;
}

// Returns pointer to a list of adjacent Nodes
Node* get_adjacents(Node node) {
    
    Node* result = malloc(sizeof(Node) * 8);
    int j = 0;
    
    for (int i = 0; i < 8; i++) {
        
        Node adjacent;
        
        if (((adjacent.row = node.row + neighborHelper[j]) < 0) || adjacent.row >= height) {
            
            adjacent.row = -1;
        }
        if (((adjacent.col = node.col + neighborHelper[j + 1]) < 0) || adjacent.col >= width) {
            
            adjacent.row = -1;
        }
        j += 2;
        result[i] = adjacent;
    }
    return result;
}

// returns char cast to the int or 0 for false
// return value depending on the state of the alive neighbors
int process_cell(Master* master, Node cell) {
    
    Node *adjacents = get_adjacents(cell);
    int count = 0;
    int max = 0;
    int current = (int) master->grid[cell.row][cell.col];
    
    for (int i = 0 ; i < 8; i++) {
        
        if (adjacents[i].row != -1) {
            
            if (master->grid[adjacents[i].row][adjacents[i].col] != '0') {
                
                if ((int)master->grid[adjacents[i].row][adjacents[i].col] > max) {
                    
                    max = (int)master->grid[adjacents[i].row][adjacents[i].col];
                }
                count++;
            }
        }
    }
    
    if (current == max) {
        
        max++;
    }
    
    if (max == (int) '5') {
        
        max--;
    }
    
    if (cell.alive) {
        
        if (count < 2) {
            
            return 0;
            
        } else if (count == 2 || count == 3) {
            
            return max;
            
        } else {
            
            return 0;
        }
    } else {
        
        if (count == 3) {
            
            return max;
            
        } else {
            
            return 0;
        }
    }
}

// Row control Thread, 20 of em running 1 per row
void* row_control(void *masterPointer) {

    Master *master = (Master *)masterPointer;

    // get id
    unsigned int my_id = get_id(master);
    // create ticks for 1000 turns temporarily this can be increased for longer game
    bool my_ticks[1000];
    for (int i = 0; i < 1000; i++) {
        // make ticks false for every elemnt
        my_ticks[i] = false;
    }
    
    // start game loop
    while (true) {
        // first check if no lock then check goAhead if turn is ready to be played
        // finally check if we already played this turn
        if (!master->lock && master->goAhead[master->turn] && !my_ticks[master->turn]) {
            // lock it
            master->lock = true;
        } else {
            continue;
        }
        // iterate over our row
        for (int i = 0; i < width; i++) {
            // create cell and check if we are alive or not
            Node cell;
            cell.row = (int) my_id;
            cell.col = i;
            if (master->grid[cell.row][cell.col] == '0') {
                cell.alive = false;
            } else {
                cell.alive = true;
            }
            // process cell, returns true if alive
            int state = process_cell(master,cell);
            if (state) {
                // update state on temp grid
                master->temp[cell.row][cell.col] = (char) state;
            } else {
                master->temp[cell.row][cell.col] = '0';
            }
        }
        // thread played its turn so tick master and tick thread
        master->tickBox[master->turn][my_id] = true;
        my_ticks[master->turn] = true;
        // unlock and wait next turn
        master->lock = false;
        sleep(1);
    }
    return NULL;
}

// Game control thread, updates the grid 
// from reading temp grid that other threads modified
void *game_control(void* masterPointer) {
    
    Master *master = (Master *)masterPointer;
    // start game thread loop
    while (true) {
        // check if game on
        while (master->gameOn) {
            // get in queue
            if (!master->lock) {
                master->lock = true;
            } else {
                continue;
            }
            // print grid and start the current turn (starts from 0)
            //print_grid(master);
            master->goAhead[master->turn] = true;
            // make turn incomplete and wait other threads 
            master->turnComplete = false;
            master->lock = false;
            sleep(1);
            // when turn complete
            while (!master->turnComplete) {
                // get in queue
                if (!master->lock) {
                    master->lock = true;
                } else {
                    continue;
                }
                // count ticks to make sure all threads played their turn
                int count = 0;
                for (int i = 0; i < height; i++) {
                    if (master->tickBox[master->turn][i]) {
                        count++;
                    }
                }
                // count matches
                if (count == height) {
                    for (int i = 0; i < height; i++) {
                        for (int j = 0; j < width; j++) {
                            // update game grid from temp grid
                            master->grid[i][j] = master->temp[i][j];
                        }
                    }
                    // complete turn and increment turn count
                    master->turn++;
                    master->turnComplete = true;
                    
                    master->lock = false;
                }
            }
        }
    }
    return NULL;
}

// Thread that send the grid info to the display program
void *display_control(void *masterPointer) {
    
    Master *master = (Master *)masterPointer;
    
    close(master->displayPipe[0]);

    while (true) {
        
        print_grid_to_pipe(master);
        
        sleep(1);
    }
    return NULL;
}

// Returns alive cell count of organism
int count_alive_cell(Master *master, Organism *organism) {
    
    int count = 0;
    for (int i = 0; i < organism->length; i++) {
        
        if (master->grid[organism->nodes[i].row][organism->nodes[i].col] != '0') {
            
            count++;
        }
    }
    return count;
}

// returns orginial alive cell count
int count_original_alive_cell(Master *master, Organism *organism) {
    
    int count = 0;
    for (int i = 0; i < organism->originalLength; i++) {
        
        if (master->grid[organism->originalNodes[i].row][organism->originalNodes[i].col] != '0') {
            
            count++;
        }
    }
    return count;
}

// returns true if the Node array contains node
bool contains_node(Node *nodes, int length, Node node) {
    
    for (int i = 0; i < length; i++) {
        
        if (nodes[i].row == node.row && nodes[i].col == node.col) {
            
            return true;
        }
    }
    return false;
}

// tries to remove the dead organism from the master
void remove_dead_organisms(Master *master, int count) {
    
    if (count == 0) {
        return;
    }
    Organism *organisms = malloc(sizeof(Organism) * 1000);
    int newLen = 0;
    
    for (int i = 0; i < master->organismAmount; i++) {
        
        if (master->organisms[i].alive) {
            
            organisms[newLen++] = master->organisms[i];
        }
    }
    for (int i = 0; i < newLen; i++) {
        
        // announce to other process if his organisms id changed
        printf("%d %d change\n", organisms[i].organismId, i);
        fflush(stdout);
        organisms[i].organismId = i;
    }
    
    free(master->organisms);
    master->organisms = organisms;
    master->organismAmount = newLen;
}

// Enlarges organism if organism have live neighbors
void enlarge_organisms(Master *master) {
    
    for (int i = 0; i < master->organismAmount; i++) {
        
        for (int j = 0; j < master->organisms[i].length; j++) {
            Node *adjacents = get_adjacents(master->organisms[i].nodes[j]);

            for (int k = 0; k < 8; k++) {
                
                if (adjacents[k].row != -1) {
                    
                    if (master->grid[adjacents[k].row][adjacents[k].col] != '0') {
                        
                        Node node;
                        node.alive = true;
                        node.row = adjacents[k].row;
                        node.col = adjacents[k].col;
                        
                        if (!contains_node(master->organisms[i].nodes, master->organisms[i].length, node)) {
                            
                            master->organisms[i].nodes[master->organisms[i].length++] = node;
                            master->organisms[i].evolved = true;
                        }
                    }
                }
            }
        }
        
    }
}

// Anounces alive organisms
void announce_alives(Master *master) {
    
    for (int i = 0; i < master->organismAmount; i++) {
        
        if (master->organisms[i].evolved) {
            
            printf("%devolved\n", master->organisms[i].organismId);
        } else {
            printf("%dalive\n", master->organisms[i].organismId);
        }
        fflush(stdout);
        
    }
}

// Updates collision boxes of organisms 
void update_collision_boxes(Master *master) {
    
    for (int i = 0; i < master->organismAmount; i++) {
        
        int lowestCol = 19;
        int lowestRow = 19;
        int highestCol = 0;
        int highestRow = 0;
        
        for (int j = 0; j < master->organisms[i].length; j++) {
            
            if (master->organisms[i].nodes[j].row > highestRow) {
                
                highestRow = master->organisms[i].nodes[j].row;
            }
            if (master->organisms[i].nodes[j].row < lowestRow) {
                
                lowestRow = master->organisms[i].nodes[j].row;
            }
            if (master->organisms[i].nodes[j].col < lowestCol) {
                
                lowestCol = master->organisms[i].nodes[j].col;
            }
            if (master->organisms[i].nodes[j].col > highestCol) {
                
                highestCol = master->organisms[i].nodes[j].col;
            }
        }
        
        master->organisms[i].botRightNode.row = highestRow;
        master->organisms[i].botRightNode.col = highestCol;
        master->organisms[i].topLeftNode.row = lowestRow;
        master->organisms[i].topLeftNode.col = lowestCol;
    }
}

// Finds collision if there is collision between 2 organism
void find_collision(Master *master) {

    for (int i = 0; i < master->organismAmount; i++) {
        
        for (int j = 0; j < master->organismAmount; j++) {
            
            if (i == j) {
                continue;
            }
            if (master->organisms[i].topLeftNode.row <= master->organisms[j].botRightNode.row && 
                    master->organisms[i].topLeftNode.col <= master->organisms[j].botRightNode.col) {
                        
                master->organisms[i].evolved = true;
            }
            if (master->organisms[i].botRightNode.row >= master->organisms[j].topLeftNode.row &&
                    master->organisms[i].botRightNode.col >= master->organisms[j].topLeftNode.col) {
                        
                master->organisms[i].evolved = true;
            }
        }
    }
}

// Thread that controls organisms
void *organism_control(void *masterPointer) {
    
    Master *master = (Master *)masterPointer;
    while (true) {
        
        if (master->organismAmount > 0) {
            
            int count = 0;
            for (int i = 0; i < master->organismAmount; i++) {
                
                if (master->organisms[i].type <= 0) {
                    if (!master->organisms[i].alive) {
                        
                        printf("%ddied\n", master->organisms[i].organismId);
                        fflush(stdout);
                        count++;
                        continue;
                    } else {
                       continue; 
                    }
                }
                int aliveCells = count_alive_cell(master, &master->organisms[i]);  
                int originalAliveCells = count_original_alive_cell(master, &master->organisms[i]);
                
                if (originalAliveCells == 0 && aliveCells == 0) {
                    
                    master->organisms[i].alive = false;
                    printf("%ddied\n", master->organisms[i].organismId);
                    fflush(stdout);
                    count++;
                }

            }
            remove_dead_organisms(master, count);
            enlarge_organisms(master);
            announce_alives(master);
            update_collision_boxes(master);
            
        }
        sleep(1);
        
    }
    return NULL;
}
