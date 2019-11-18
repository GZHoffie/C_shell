//
// Created by 顾振昊 on 2019/9/29.
//

#include "builtin.h"
#include <stdio.h>
#include  <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include  <unistd.h>
#include <stdlib.h>

#define MAXSIZE 1024

int mumsh_builtin(char ** args){
    if (strcmp(args[0], "pwd") == 0){
        pwd();
        return 1;
    }
    return 0;
}

void pwd(){
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    //fprintf(stderr, "executing pwd\n");
    fprintf(stdout, "%s\n", cwd);
    fflush(stdout);
}

void cd(char * dir){
    if (chdir(dir) < 0){
        printf("%s: No such file or directory\n", dir);
        fflush(stdout);
        return;
    }
    //char cwd[1024];
    //getcwd(cwd, sizeof(cwd));
    //fprintf(stderr, "executing cd\n");
    //fprintf(stdout, "%s\n", cwd);
}