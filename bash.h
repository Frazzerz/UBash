#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<errno.h>
#include<fcntl.h>

/////////////////////////////////

#define MAXINPUT 1000											// Massima dimensione input
#define MAXCMD 100												// Massimi comandi possibili
#define MAXARG 10												// Massimi argomenti per ogni comando possibili

/////////////////////////////////

extern void printDir();
extern int takeinput(char* str);
extern int parsePipe(char* input, char** tokens[]);
extern void execcmd(char** cmd, pid_t pid);
extern void execpipe(char** cmd[], int n, pid_t pid);

////////////////////////////////