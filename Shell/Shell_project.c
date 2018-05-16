/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)
**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <libgen.h>
#include "commands.c"
#include <string.h>
#define SHELL "shell"
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

char* status_info(int status, int info);
// ----------------------------------------------------------------------- //
//                            MAIN          							   //
// ----------------------------------------------------------------------- //

int main(void){
	ignore_terminal_signals();	/*Ignore SIGINT SIGQUIT SIGTSTP SIG TTIN SIGTTOU signals*/
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; 	/* pid for created and waited process */
	int status;             	/* status returned by wait */
	enum status status_res; 	/* status processed by analyze_status() */
	int info;					/* info processed by analyze_status() */
    char* status_res_str, *aux;
	printf("Welcome to the Shell\n\n");
	new_process_group(getpid()); //Process group Tarea 2
	while(1){   				/* Program terminates normally inside get_command() after ^D is typed*/
		ignore_terminal_signals();	/*Ignore SIGINT SIGQUIT SIGTSTP SIG TTIN SIGTTOU signals*/
		aux = strdup(getenv("PWD"));
		printf("[%s@shell:%s]$ ", getenv("USER"), basename(aux));
		free(aux);
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command
        if(!isAShellOrder(args[0], args)) {
            pid_fork = fork();
            if (pid_fork) {
                new_process_group(pid_fork);
                if (!background) {    //Checks if it's in background
                    //FATHER
                    set_terminal(pid_fork);
                    waitpid(pid_fork, &status, 0);
                    set_terminal(getpid());
                }
            } else {/*FOREGROUND COMMAND. SON*/
                //Restores the signals here because this is the forked process.
                //The father is immune to the signals, this code does not
                //Modify the father behaviour.
                restore_terminal_signals();
                exit(execvp(args[0], args));
            }

            if (WEXITSTATUS(status) != 0) {
                printf("Error, command not found: %s\n", args[0]);
            } else if (background) {/*BACKGROUND COMMAND*/
                printf("\nBackground running job... pid: %d, command %s\n", pid_fork, args[0]);
                status_res_str = status_info(status, info);
            } else {
                status_res_str = status_info(status, info);
                printf("\nForeground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_res_str, info);
            }
        }
		/*
		   the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

	} // end while
}

char* status_info(int status, int info){
	char* status_res_str;
	int status_res;
	status_res = analyze_status(status, &info);

    if(status_res == 0){
        status_res_str = "SUSPENDED";
    }else if(status_res == 1){
        status_res_str = "SIGNALED";
    }else if(status_res == 2){
        status_res_str = "EXITED";
    }else if(status_res == 4){
    	status_res_str = "CONTINUED";
    }

    return status_res_str;
}