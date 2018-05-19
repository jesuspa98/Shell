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
#include "commands.c"		//extra file to internal commands such as "cd"
#include <string.h>
#include <signal.h>
//Simplemente por estetica
#define Shell "\033[31mShell\033[0m"
#define Pid "\033[31mpid\033[0m"
#define Command "\033[31mcommand\033[0m"
#define Info "\033[31minfo\033[0m"
//Hasta aqui
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
job* job_list;
void SIGCHLD_handler();
int isAShellOrder(char* commandName, char* argsv[]);
// ----------------------------------------------------------------------- //
//                            MAIN          							   //
// ----------------------------------------------------------------------- //

int main(void){
	ignore_terminal_signals();	/*Ignore SIGINT SIGQUIT SIGTSTP SIG TTIN SIGTTOU signals*/
	job_list = new_list("Shell jobs");
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; 	/* pid for created and waited process */
	int status;             	/* status returned by wait */
	enum status status_res; 	/* status processed by analyze_status() */
	int info, goid_father;		/* info processed by analyze_status() */
    char* status_res_str;		//Auxiliar definition
    //-- START CODE --//
	new_process_group(getpid()); //Process group Tarea 2
	printf("Welcome to the %s\n\n", Shell);
    signal(SIGCHLD, SIGCHLD_handler);
	while(1){   				/* Program terminates normally inside get_command() after ^D is typed*/
		{
			char currentPath[255];
			getcwd(currentPath, 255); //Current path, simply aesthetic.
			printf("[%s@%s:%s]$ ", getenv("USER"), Shell, basename(currentPath));
			fflush(stdout);
		}

		block_SIGCHLD();
        get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

        if(args[0]==NULL){
            unblock_SIGCHLD();
            continue;   // if empty csommand
        }

        if(!isAShellOrder(args[0], args)){
            pid_fork = fork();

            if(pid_fork){
                // FATHER.

                if(!background){ // Checks if the command eecuted is a background command.
                    unblock_SIGCHLD();
                    set_terminal(pid_fork);
                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                    set_terminal(getpid());
                }

                // STATUS MESSAGES THAT THE SHELL SENDS TO THE USER.
                status_res = analyze_status(status, &info);
                status_res_str = status_strings[status_res];

                if(WEXITSTATUS(status) == 127){
                    printf("Error command not found: %s\n", args[0]);
                }else if(background){
                    add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
                    print_job_list(job_list);
                    printf("Background running job... %s: %d, %s: %s\n", Pid, pid_fork, Command, args[0]);
                }else if(status_res == SUSPENDED){
                    printf("\nForeground %s: %d, %s: %s, has been suspended...\n", Pid, pid_fork, Command, args[0]);
                    add_job(job_list, new_job(pid_fork, args[0], STOPPED));
                    set_terminal(getpid());
                }else{
                    printf("\nForeground %s: %d, %s: %s, %s, %s: %d\n", Pid, pid_fork, Command, args[0], status_res_str, Info, info);
                }

                unblock_SIGCHLD();

            }else{
                //Restores the signals here because this is the forked process.
                //The father is immune to the signals, this code does not
                //Modify the father behaviour.
                unblock_SIGCHLD();
                /* FOREGROUND COMMANDS. SON. */
                new_process_group(getpid());

                if(!background){
                    set_terminal(getpid());
                }

                restore_terminal_signals();
                execvp(args[0], args);
                exit(127);
            }
        }
    } // end while
}//end main

void SIGCHLD_handler(){
    int exitStatus, info, hasToBeDeleted, n = 0;
    job* job = job_list->next;
    enum status status_res;
    pid_t pid;
    while(job != NULL){
        hasToBeDeleted = 0;
        pid = waitpid(job->pgid, &exitStatus, WNOHANG | WUNTRACED);
        if (pid == job->pgid){
            status_res = analyze_status(exitStatus, &info);
            if(status_res == EXITED) {
                hasToBeDeleted = 1;
                printf(" [%d]+ Done\n", n);
            } else if(job->state != STOPPED && status_res == SUSPENDED) {
                //Si se ha suspendido, hay que anotarlo y notificarlo
                job->state = STOPPED;
                printf(" - %d %s has been suspended\n", job->pgid, job->command);
            }
        }
        if(hasToBeDeleted) {
            struct job_* jobToBeDeleted = job;
            job = job->next;
            delete_job(job_list, jobToBeDeleted);
        } else {
            job = job->next;
        }
        n++;
    }
}//end SIGCHLD_handler