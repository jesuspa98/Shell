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
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "job_control.h"   // remember to compile with module job_control.c

#define Shell "\033[31mShell\033[0m"
#define Pid "\033[31mpid\033[0m"
#define Command "\033[31mcommand\033[0m"
#define Info "\033[31minfo\033[0m"
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

/**
 * Other Functions.
 */
int isAShellOrder(char *commandName, char *argsv[]);

void SIGCHLD_handler(int signal);

/**
 * Other Variables.
*/
typedef struct commands {
    char *name;
} ShellCommands;
static const ShellCommands shellCommands[] = {{"cd"}};
job *job_list;
// ---------------------------------------------------------------------------//
//                            		MAIN          							  //
// ---------------------------------------------------------------------------//

int main(void) {
    ignore_terminal_signals();  // Ignores SIGINT SIGQUIT SIGTSTP SIGTTIN SIGTTOU signals.
    job_list = new_list("Shell Jobs");
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2];     /* command line (of 256) has max of 128 arguments */
    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;             /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;                /* info processed by analyze_status() */
    char *status_res_str;

    new_process_group(getpid()); // PROCESS GROUP 'TAREA' 2
    signal(SIGCHLD, SIGCHLD_handler);

    while (1) {  /* Program terminates normally inside get_command() after ^D is typed*/
        {
            char currentPath[255];
            getcwd(currentPath, 255); //Current path, simply aesthetic.
            printf("[%s@%s:%s]$ ", getenv("USER"), Shell, basename(currentPath));
            fflush(stdout);
        }

        block_SIGCHLD();
        get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */

        if (args[0] == NULL) {
            unblock_SIGCHLD();
            continue;   // if empty command
        }
        if (!isAShellOrder(args[0], args)) {
            pid_fork = fork();

            if (pid_fork) {//Father
                new_process_group(pid_fork);

                if (!background) {
                    unblock_SIGCHLD();
                    set_terminal(pid_fork);
                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                    set_terminal(getpid());
                } else {
                    add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
                }

                status_res = analyze_status(status, &info);
                status_res_str = status_strings[status_res];

                if (WEXITSTATUS(status) != 0) {
                    printf("\nError, command not found: %s\n", args[0]);
                } else if (background) {
                    printf("Background running job... %s: %d, %s: %s\n", Pid, pid_fork, Command, args[0]);
                } else {
                    printf("\nForeground %s: %d, %s: %s, %s, %s: %d\n", Pid, pid_fork,
                           Command, args[0], status_res_str, Info, info);
                }
                unblock_SIGCHLD();

            } else {//Son
                unblock_SIGCHLD();
                new_process_group(getpid());
                if (!background) {
                    set_terminal(getpid());
                }
                restore_terminal_signals();
                exit(execvp(args[0], args));
            }


        }
    } // end while
}

int isAShellOrder(char *commandName, char *args[]) {
    int i = 0, equals = 0;

    size_t length = sizeof(shellCommands) / sizeof(shellCommands[0]);

    while (!equals && i < length) {
        equals = strcmp(commandName, shellCommands[i].name);
        i++;
    }

    if (!equals) {
        chdir(args[1]);
    }

    return !equals;
}

void SIGCHLD_handler(int signal) {
    /* TODO
     * Al  activarse  el  manejador  de  la  señal  SIGCHLD  habrá  que  revisar  cada  entrada  de
     * la  lista  para comprobar si algún proceso en segundo plano ha terminado o se ha suspendido.
     * Para comprobar si un proceso ha terminado sin bloquear al Shell hay que añadir la opción WNOHANG en
     * la función
     * waitpid
     */
    int hasToBeDeleted, exitStatus, number = 0, info;
    enum status status_res;
	job* job = job_list->next;
    pid_t pid;

    while(job != NULL){
        hasToBeDeleted = 0;
        pid = waitpid(job->pgid, &exitStatus, WNOHANG | WUNTRACED);

        if(pid == job->pgid){
            status_res = analyze_status(exitStatus, &info);

            if(status_res == EXITED){
                hasToBeDeleted = 1;
                printf("[%d]+ Done\n", number);
            }else if(job->state != STOPPED && status_res == SUSPENDED){
                job->state = STOPPED;
                printf(" - %d %s has been suspended\n", job->pgid, job->command);
            }else if(job->state == STOPPED && status_res == CONTINUED){
                job->state = BACKGROUND;
                printf(" - %d %s has been continued\n", job->pgid, job->command);
            }
        }

        if(hasToBeDeleted){
            struct job_* jobToBeDeleted = job;
            job = job->next;
            delete_job(job_list, jobToBeDeleted);
        }else{
            job = job->next;
        }
        number++;
    }
}