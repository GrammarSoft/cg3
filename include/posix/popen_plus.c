/*
 ** Author: Hamid Alipour http://codingrecipes.com http://twitter.com/code_head
 ** SQLite style license:
 ** 
 ** 2001 September 15
 **
 ** The author disclaims copyright to this source code.  In place of
 ** a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **/

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "popen_plus.h"

popen_plus_process *popen_plus(const char *command)
{
    int inpipe[2];
    int outpipe[2];
    char *argv[4];
    popen_plus_process *process = (popen_plus_process*)malloc(sizeof(popen_plus_process));
    
    if (!process)
        goto error_out;
    
    if (pipe(inpipe) != 0)
        goto clean_process_out;
    
    if (pipe(outpipe) != 0)
        goto clean_inpipe_out;
    
    process->read_fp = fdopen(outpipe[READ], "r");
    if (!process->read_fp)
        goto clean_outpipe_out;
    
    process->write_fp = fdopen(inpipe[WRITE], "w");
    if (!process->write_fp)
        goto clean_read_fp_out;
    
    if (pthread_mutex_init(&process->mutex, NULL) != 0)
        goto clean_write_fp_out;
    
    process->pid = fork();
    if (process->pid == -1)
        goto clean_mutex_out;
    
    if (process->pid == 0) {
        close(outpipe[READ]);
        close(inpipe[WRITE]);
        
        if (inpipe[READ] != STDIN_FILENO) {
            dup2(inpipe[READ], STDIN_FILENO);
            close(inpipe[READ]);
        }
        
        if (outpipe[WRITE] != STDOUT_FILENO) {
            dup2(outpipe[WRITE], STDOUT_FILENO);
            close(outpipe[WRITE]);
        }
        
        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = (char *) command;
        argv[3] = NULL;
        
        execv(_PATH_BSHELL, argv);
        exit(127);
    }
    
    close(outpipe[WRITE]);
    close(inpipe[READ]);
    
    return process;
    
clean_mutex_out:
    pthread_mutex_destroy(&process->mutex);
    
clean_write_fp_out:
    fclose(process->write_fp);
    
clean_read_fp_out:
    fclose(process->read_fp);
    
clean_outpipe_out:
    close(outpipe[READ]);
    close(outpipe[WRITE]);
    
clean_inpipe_out:
    close(inpipe[READ]);
    close(inpipe[WRITE]);
    
clean_process_out:
    free(process);
    
error_out:
    return NULL;
}

int popen_plus_close(popen_plus_process *process)
{
    int pstat = 0;
    pid_t pid = 0;
    
    /**
     * If someone else destrys this mutex, then this call will fail and we know
     * that another thread already cleaned up the process so we can safely return
     * and since we are destroying this mutex bellow then we don't need to unlock
     * it...
     */
    if (pthread_mutex_lock(&process->mutex) != 0)
        return 0;
        
    if (process->pid != -1) {
        do {
            pid = waitpid(process->pid, &pstat, 0);
        } while (pid == -1 && errno == EINTR);
    }
    
    if (process->read_fp)
        fclose(process->read_fp);
    
    if (process->write_fp)
        fclose(process->write_fp);
    
    pthread_mutex_destroy(&process->mutex);
    
    free(process);
    
    return (pid == -1 ? -1 : pstat);
}

int popen_plus_kill(popen_plus_process *process)
{
    char command[64];
    
    sprintf(command, "kill -9 %d", process->pid);
    system(command);
    
    return 0;
}

int popen_plus_kill_by_id(int process_id)
{
    char command[64];
    
    sprintf(command, "kill -9 %d", process_id);
    system(command);
    
    return 0;
}

int popen_plus_terminate(popen_plus_process *process)
{
    char command[64];
    
    sprintf(command, "kill -TERM %d", process->pid);
    system(command);
    
    return 0;
}

int popen_plus_terminate_with_id(int process_id)
{
    char command[64];
    
    sprintf(command, "kill -TERM %d", process_id);
    system(command);
    
    return 0;
}
