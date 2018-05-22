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
            if(!background){
                waitpid(pid_fork, &status, 0);
                if(WEXITSTATUS(status)){
                    printf("Error, command not found: %s\n", args[0]);
                }else{
                    status_res = analyze_status(status, &info);
                    if(status_res == 0){
                        status_res_str = "SUSPENDED";
                    }else if(status_res == 1){
                        status_res_str = "SIGNALED";
                    }else if(status_res == 2){
                        status_res_str = "EXITED";
                    }else if(status_res == 3){
                        status_res_str = "CONTINUED";
                    }
                    printf("\nForeground %s: %d, %s: %s, %s, %s: %d\n",
                           Pid, pid_fork, Command, args[0], status_res_str, Info, info);
                }
            }else{
                    printf("\nBackground job running... %s: %d, %s: %s\n", Pid, pid_fork, Command, args[0]);
            }
        }else{
            exit(execvp(args[0], args));
        }

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

	} // end while
}
