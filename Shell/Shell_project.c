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

#include <libgen.h>
#include "job_control.h"   // remember to compile with module job_control.c
#define Shell "\033[31mShell\033[0m"
#define Pid "\033[31mpid\033[0m"
#define Command "\033[31mcommand\033[0m"
#define Info "\033[31minfo\033[0m"
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void) {
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
    char* status_res_str;

	while (1) {  /* Program terminates normally inside get_command() after ^D is typed*/
        {
            char currentPath[255];
            getcwd(currentPath, 255); 							//Current path, simply aesthetic.

            printf("[%s@%s:%s]$ ", getenv("USER"), Shell, basename(currentPath));
            fflush(stdout);
        }
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command
        pid_fork = fork();
        if(pid_fork){

            if(!background) {
                pid_wait = waitpid(pid_fork, &status, 0);
            }

            status_res = analyze_status(status, &info);
            status_res_str = status_strings[status_res];

            if(WEXITSTATUS(status) == 127){
                printf("\nError, command not found: %s\n", args[0]);
            }else if(background){
                printf("Background running job... %s: %d, %s: %s\n", Pid, pid_fork, Command, args[0]);
            }else{
                printf("\nForeground %s: %d, %s: %s, %s, %s: %d\n", Pid, pid_fork,
                       Command, args[0], status_res_str, Info, info);
            }
        }else{
            exit(execvp(args[0], args));
        }
	} // end while
}
