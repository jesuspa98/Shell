#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "job_control.h"

typedef struct commands_{
	char* name;
	void (*func)(int args, char* argsv[]);
} ShellCommands;

extern job* job_list;
static void cd(int args, char* argsv[]);
static void jobs(int args, char* argsv[]);
static void fg(int args, char* argsv[]);
static void bg(int args, char* argsv[]);
static int args(char* argsv[]);
static const ShellCommands shellCommands[] = { {"cd", cd}, {"jobs", jobs}, {"fg", fg}, {"bg", bg} };

int isAShellOrder(char*commandName, char* argsv[]){
    int notEquals = 1, i = 0;
    size_t length = sizeof(shellCommands)/ sizeof(shellCommands[0]);
    while(notEquals && i < length){
        notEquals = strcmp(commandName, shellCommands[i].name);
        i++;
    }

    if(!notEquals){
        shellCommands[i - 1].func(args(argsv), argsv);
    }

    return !notEquals;
}

void cd(int args, char* argsv[]){
    //acceder a un directorio
    if(args == 1){
        chdir("/home/jesuspa98");
    }else{
        if(chdir(argsv[1]) == -1){
            perror(argsv[1]);
        }
    }
}
void jobs(int args, char* argsv[]){
    //mostrar los jobs actuales
    print_job_list(job_list);
}

void fg(int args, char* argsv[]){
    //Poner una tarea de las que esta en segundo plano o suspendida en primer plano
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

        //Cedemos la terminal al grupo de procesos...
        set_terminal(job->pgid);

        //...y les hacemos notificar de ello para que puedan ejecutarse sin problemas
        printf("%s\n", job->command);
        killpg(job->pgid, SIGCONT);
        unblock_SIGCHLD();
        pid_t p = waitpid(job->pgid, &status, WUNTRACED);
        set_terminal(getpid());
        block_SIGCHLD();
        status_res = analyze_status(status, &info);

        if(status_res == SUSPENDED){
            //Si la tarea ha sido suspendida otra vez, lo hacemos notificar al usuario y cambiamos el estado del trabajo
            printf("\n%s supsended with code %d\n", job->command, info);
            job->state = STOPPED;
        }else{
            //Si se ha cerrado el proceso, se lo hacemos saber al usuario y eliminamos el trabajo de la lista
            printf("\n%s exited with status: %s, %d\n", job->command, status_strings[status_res], info);
            delete_job(job_list, job);
        }
    }else{
        printf("Job %d doesn't exists\n", num);
    }

    unblock_SIGCHLD();
}

void bg(int args, char* argsv[]){
	 block_SIGCHLD();
    int num;

    if(args != 1) {
        num = atoi(argsv[1]);
    } else {
        num = 1;
    }
    job* job = get_item_bypos(job_list, num);
    if(job != NULL) {
        job->state = BACKGROUND;    		// Modify the job state to backgroud
        killpg(job->pgid, SIGCONT); 		// Send signal SIGCONT to the process group job-pgid
    } else {
        printf("Job %d doesn't exists\n", num);
    }

    unblock_SIGCHLD();
}

int args(char* argsv[]){
    //cantidad de argumentos que tiene argsv
    int args = 0;

    while(argsv[args] != NULL){
        args++;
    }

    return args;
}