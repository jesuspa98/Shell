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
*/

/**
 * All includes
 */
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "job_control.h"   // remember to compile with module job_control.c

/**
 * All defines
 */
#define Shell "\033[31mShell\033[0m"
#define Pid "\033[31mpid\033[0m"
#define Command "\033[31mcommand\033[0m"
#define Info "\033[31minfo\033[0m"
#define Status "\033[31mstatus\033[0m"
#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

/**
 * Other Variables.
*/
typedef struct commands {
    char *name;

    void (*func)(int args, char *argsv[]);
} ShellCommands;
job *job_list;

/**
 * Other Functions.
 */
int isAShellOrder(char *commandName, char *argsv[]);

void SIGCHLD_handler(int signal);

static void cd(int args, char *argsv[]);

static void jobs(int args, char *argsv[]);

static void fg(int args, char *argsv[]);

static void bg(int args, char *argsv[]);

static int args(char *argsv[]);

static void exits(int args, char *argsv[]);

static const ShellCommands shellCommands[] = {{"cd", cd}, {"jobs", jobs}, {"fg", fg}, {"bg", bg}, {"exit", exits}};
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
    printf("Welcome to the %s\n\n", Shell);
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
                if (!background) {
                    unblock_SIGCHLD();
                    set_terminal(pid_fork);
                    pid_wait = waitpid(pid_fork, &status, WUNTRACED);
                    set_terminal(getpid());
                } /*else {
                    add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
                }*/

                status_res = analyze_status(status, &info);
                status_res_str = status_strings[status_res];

                if (WEXITSTATUS(status) == 127) {
                    printf("\nError, command not found: %s\n", args[0]);
                } else if (background) {
                    add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
                    print_job_list(job_list);
                    printf("Background running job... %s: %d, %s: %s\n", Pid, pid_fork, Command, args[0]);
                } else if (status_res == SUSPENDED) {
                    printf("\nForeground %s: %d, %s: %s, has been suspended...\n", Pid, pid_fork, Command, args[0]);
                    add_job(job_list, new_job(pid_fork, args[0], STOPPED));
                    set_terminal(getpid());
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
                execvp(args[0], args);
                exit(127);
            }
        }
    } // end while
}

int isAShellOrder(char *commandName, char *argsv[]) {
    int i = 0, equals = 1;

    size_t length = sizeof(shellCommands) / sizeof(shellCommands[0]);

    while (equals && i < length) {
        equals = strcmp(commandName, shellCommands[i].name);
        i++;
    }

    if (!equals) {
        shellCommands[i - 1].func(args(argsv), argsv);
    }

    return !equals;
}

void SIGCHLD_handler(int signal) {
    int hasToBeDeleted, exitStatus, number = 0, info;
    enum status status_res;
    job *job = job_list->next;
    pid_t pid;

    while (job != NULL) {
        hasToBeDeleted = 0;
        pid = waitpid(job->pgid, &exitStatus, WNOHANG | WUNTRACED);

        if (pid == job->pgid) {
            status_res = analyze_status(exitStatus, &info);

            if (status_res == EXITED) {
                hasToBeDeleted = 1;
                printf("[%d]+ Done\n", number);
            } else if (job->state != STOPPED && status_res == SUSPENDED) {
                job->state = STOPPED;
                printf(" - %d %s has been suspended\n", job->pgid, job->command);
            } else if (job->state == STOPPED && status_res == CONTINUED) {
                job->state = BACKGROUND;
                printf(" - %d %s has been continued\n", job->pgid, job->command);
            }
        }

        if (hasToBeDeleted) {
            struct job_ *jobToBeDeleted = job;
            job = job->next;
            delete_job(job_list, jobToBeDeleted);
        } else {
            job = job->next;
        }
        number++;
    }
}

static void cd(int args, char *argsv[]) {
    if (args == 1) {
        chdir("/home/jesuspa98");
    } else {
        if (chdir(argsv[1]) == -1) {
            perror(argsv[1]);
        }
    }
}

static void jobs(int args, char *argsv[]) {
    print_job_list(job_list);
}

static void fg(int args, char *argsv[]) {
    unblock_SIGCHLD();
    int num;

    if(args != 1) {
        num = atoi(argsv[1]);
    }else{
        num = 1;
    }

    job* job = get_item_bypos(job_list, num);

    if(job != NULL){
        int status, info;
        enum status status_res;

        job->state = FOREGROUND;
        set_terminal(job->pgid);//Cedemos la terminal al grupo de procesos...
        printf("%s: %s in foreground\n", Command, job->command);
        killpg(job->pgid, SIGCONT);
        unblock_SIGCHLD();
        pid_t p = waitpid(job->pgid, &status, WUNTRACED);
        set_terminal(getpid());
        block_SIGCHLD();
        status_res = analyze_status(status, &info);

        if (status_res == SUSPENDED) {
            //Si la tarea ha sido suspendida otra vez.
            printf("\n%s supsended with code %d\n", job->command, info);
            job->state = STOPPED;
        } else {
            //Si se ha cerrado el proceso.
            printf("\n%s exited with status: %s, %d\n", job->command, status_strings[status_res], info);
            delete_job(job_list, job);
        }
    } else {
        printf("Job %d doesn't exists\n", num);
    }

    unblock_SIGCHLD();
}

static void bg(int args, char *argsv[]) {
    block_SIGCHLD();
    int number;

    if (args != 1) {
        number = atoi(argsv[1]);
    } else {
        number = 1;
    }

    job *job = get_item_bypos(job_list, number);

    if (job != NULL) {
        job->state = BACKGROUND;
        killpg(job->pgid, SIGCONT);
    } else {
        printf("Job %s doesn't exists\n", number);
    }

    unblock_SIGCHLD();
}

static int args(char *argsv[]) {
    int number = 0;
    while (argsv[number] != NULL) {
        number++;
    }
    return number;
}

static void exits(int args, char *argsv[]) {
    exit(0);
}