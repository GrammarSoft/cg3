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

#pragma once
#ifndef POPEN_PLUS_H_f28c53c53a48d38efafee7fb7004a01faaac9e22
#define POPEN_PLUS_H_f28c53c53a48d38efafee7fb7004a01faaac9e22

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define READ 0
#define WRITE 1

typedef struct {
    pthread_mutex_t mutex;
    pid_t pid;
    FILE *read_fp;
    FILE *write_fp;
} popen_plus_process;

popen_plus_process *popen_plus(const char *command);
int popen_plus_close(popen_plus_process *process);
int popen_plus_kill(popen_plus_process *process);
int popen_plus_kill_by_id(int process_id);
int popen_plus_terminate(popen_plus_process *process);
int popen_plus_terminate_with_id(int process_id);

#ifdef __cplusplus
}
#endif

#endif
