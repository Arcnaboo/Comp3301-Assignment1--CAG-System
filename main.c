/*
 * @course: Comp 3301
 * @task: Assignment 1
 * @author: Arda 'ARC' Akgur
 * @file: main.c
 * */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Main Loop
int main(int argc, char **argv) {
    
    char** args = malloc(sizeof(char*) * 2);
    args[0] = "./usershell";
    args[1] = (char*) 0;
    if (execvp(args[0], args) == -1) {
        fprintf(stderr, "Unable to run User Shell\n");
        exit(21);
    }
    return 0;
    
}