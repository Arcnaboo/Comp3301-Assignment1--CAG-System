/*
 * @course: Comp 3301
 * @task: Assignment 1
 * @author: Arda 'ARC' Akgur
 * @file: s4382911_cagdisplay.c
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <pthread.h>
#include <signal.h>

#define clear_terminal() printf("\033[H\033[J")
#define move_cursor(x,y) printf("\033[%d;%dH", (x), (y))

// Grid Updated Thread Function
void *grid_updater(void *masterPointer);

// Game height width
unsigned int height = 20;
unsigned int width = 20;

// Type to define Colors
typedef struct {
    
    long int red;
    long int green;
    long int blue;
} ArcColor;

// Some colors
ArcColor ared, ablue, agreen, acyan, amagenta, ayellow, aorange, aalice, ablack, awhite;

// Function that prepares color
void arrange_colors(void) {
    
    awhite.red = 0xFFFF;
    awhite.green = 0xFFFF;
    awhite.blue = 0xFFFF;
    ablack.red = 0;
    ablack.green = 0;
    ablack.blue = 0;
    ared.red = 0xFFFF;
    ared.green = 0;
    ared.blue = 0;
    ablue.red = 0;
    ablue.green = 0;
    ablue.blue = 0xFFFF;
    agreen.red = 0;
    agreen.green = 0xFFFF;
    agreen.blue = 0;
    acyan.red = 0;
    acyan.green = 0xFFFF;
    acyan.blue = 0xFFFF;
    amagenta.red = 0xFFFF;
    amagenta.green = 0;
    amagenta.blue = 0xFFFF;
    ayellow.red = 0xFFFF;
    ayellow.green = 0xFFFF;
    ayellow.blue = 0;
    aorange.red = 0xFFFF;
    aorange.green = 0xA5A5;
    aorange.blue = 0;
    aalice.red = 0xF0F0; 
    aalice.green = 0xF8F8; 
    aalice.blue = 0xFFFF;
}

// Master type to hold all data
typedef struct {
    
   	Display *thedisplay;
   	GC thecontext;
   	XColor xcolour;
   	Colormap thecolormap;
   	Window thewindow;
	XEvent anevent;
    int thescreen;
    int blackColor;
    int whiteColor;
    char **grid;
} Master;

// Setups xLib window for display
void setup_window(Master *master) {
    
     /* Setup a window */
    master->thedisplay  = XOpenDisplay(NULL);
   	master->blackColor  = BlackPixel(master->thedisplay, DefaultScreen(master->thedisplay));
   	master->whiteColor  = WhitePixel(master->thedisplay, DefaultScreen(master->thedisplay));
   	master->thescreen   = DefaultScreen(master->thedisplay);
   	master->thecolormap = DefaultColormap(master->thedisplay, master->thescreen);
    /* Create the window */
    master->thewindow = XCreateSimpleWindow(master->thedisplay, 
            DefaultRootWindow(master->thedisplay), 
            0, 0, 800, 600, 5, master->whiteColor, master->blackColor);
   	XSelectInput(master->thedisplay , master->thewindow, StructureNotifyMask);
   	XMapWindow(master->thedisplay, master->thewindow);
    XStoreName(master->thedisplay, master->thewindow, "COMP 3301 - CAG - Arda Akgur");
    /* Get the context */
    master->thecontext = XCreateGC(master->thedisplay, master->thewindow, 0, NULL);
   	XSetBackground(master->thedisplay, master->thecontext, master->blackColor);
   	XSetForeground(master->thedisplay, master->thecontext, master->whiteColor);   
}

// Sets foreground color to given ArcColor
void set_color(Master *master, ArcColor arcColor) {
    
    master->xcolour.red = arcColor.red; 
    master->xcolour.green = arcColor.green;
    master->xcolour.blue = arcColor.blue;
   	master->xcolour.flags = DoRed | DoGreen | DoBlue;
   	XAllocColor(master->thedisplay, master->thecolormap, &master->xcolour);
    XSetForeground(master->thedisplay, master->thecontext, master->xcolour.pixel);
}

// Draws an empty grid to window
void draw_empty_grid(Master *master) {
    
    set_color(master, ared);
    for (int i = 0; i < height; i++) {
        
        XDrawLine(master->thedisplay, master->thewindow, master->thecontext, i * 30, 0, i * 30, 600);
    }
    for (int i = 0; i < width; i++) {
        
        XDrawLine(master->thedisplay, master->thewindow, master->thecontext, 0, i * 30, 600, i * 30);
    }
}

// Fills the visual grid with rectangles depending on
// the master->grid
void fill_the_grid(Master *master) {
    
    for (int i = 0; i < height; i++) {
        
        for (int j = 0; j < width; j++) {
            
            if (master->grid[i][j] == '0') {
                
                continue;
            } else if (master->grid[i][j] == '1') {
                
                set_color(master, agreen);
            } else if (master->grid[i][j] == '2') {
                
                set_color(master, amagenta);
            } else if (master->grid[i][j] == '3') {
                
                set_color(master, ablue);
            } else if (master->grid[i][j] == '4') {
                
                set_color(master, aalice);
            } else if (master->grid[i][j] == '5') {
                
                set_color(master, aorange);
            } else {
                
                set_color(master, ayellow);
            }
            
            XFillRectangle(master->thedisplay, master->thewindow, 
                    master->thecontext, 30.1 * j, 30.1 * i, 29.9, 29.9);
        }
    }
}

// Draws the right side of the screen
void draw_right_side(Master *master) {
    set_color(master, ared);
    XFillRectangle(master->thedisplay, master->thewindow, 
            master->thecontext, 600, 0, 200, 800);
            
    set_color(master, ablack);
    
    XFillRectangle(master->thedisplay, master->thewindow,
            master->thecontext, 620, 0, 180, 780);
    
    set_color(master, agreen);
    XDrawString(master->thedisplay, master->thewindow, master->thecontext, 
            655, 20, "COMP3301", 8);
    XDrawString(master->thedisplay, master->thewindow, master->thecontext, 
            650, 30, "Arda Akgur", 10);
}

// Allocates memory for the char representation
// of the grid
void allocate_grid(Master *master) {
    
    master->grid = malloc(sizeof(char *) * height);
    for (int i = 0; i < height; i++) {
        
        master->grid[i] = malloc(sizeof(char) * width + 1);
        memset(master->grid[i], '0', width);
        master->grid[i][width] = '\0';
    }
    
}

// Prints grid if needed
void print_grid(Master *master) {
    
    for (int i = 0; i < height; i++) {
        
        printf("%s\n", master->grid[i]);
    }
}

// SIGINT Handler
static void ctrl_c_handler(int sig) {
    
    printf("Terminating CAG\n");
	exit(0);
}

// Main loop
int main(int agc, char** argv) {
    
    arrange_colors();
    Master master;
    struct sigaction usr1Act;
	memset(&usr1Act, 0, sizeof(struct sigaction));
	usr1Act.sa_handler = ctrl_c_handler;
	usr1Act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &usr1Act, NULL);
    setup_window(&master);
    allocate_grid(&master);
    pthread_t gridUpdater;
    
    if (pthread_create(&gridUpdater, NULL, grid_updater, &master)) {
        
            fprintf(stderr, "Error creating thread\n");
            exit(20);
    }

    for (;;) {
        
        XNextEvent(master.thedisplay, &master.anevent);
        if (master.anevent.type == MapNotify)
            
            break;
    }
   
   	/* Erase the display (In the background colour) */
   	XClearWindow(master.thedisplay, master.thewindow);
    
    
    while(1) {
        
        XClearWindow(master.thedisplay, master.thewindow);
        draw_empty_grid(&master);
        draw_right_side(&master);
        fill_the_grid(&master);
        XFlush(master.thedisplay);
        sleep(1);
    }

	return 0;
}

// checks if line is 'end'
// also returns index pos of the data
// -1 if error
int check_end(char* line) {
    
    line[strlen(line) - 1] = '\0';
    if (!strcmp(line, "end")) {
        printf("Terminating CAG\n");
        exit(0);
    }
    line[0] = ' ';
    for (int i = 0; i < strlen(line); i++) {
        if (line[i] == ',') {
            return i + 1;
        }
    }
    return -1;
}

// Grid Updater Thread runs in background
// reads stdin for the row data
// updates grid accordingly
void *grid_updater(void *masterPointer) {
    
    Master *master = (Master *)masterPointer;
    
    
    while (1) {
        
        char buffer[255];
        if (!fgets(buffer, 255, stdin)) {
            exit(0);
        }
        
        char* ptr;
        int start = check_end(buffer);
        int row = strtol(buffer, &ptr, 10);
        
        int j = 0;
        for (int i = start; i < strlen(buffer); i += 2) {
            master->grid[row][j] = buffer[i];
            j++;
        }
    }
    return NULL;
}