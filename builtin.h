//
// Created by 顾振昊 on 2019/9/29.
//

#ifndef VE482P1_BUILTIN_H
#define VE482P1_BUILTIN_H


int mumsh_builtin(char ** args);
// EFFECTS: check whether the "args" is one of the built-in function in mumsh.
//          returns 1 if the command is recognized and executed. returns 0 if
//          the command is not found. returns -1 if error occurred.

void pwd();
// EFFECTS: same as "pwd" in linux command, prints the current working directory
//          to stdout.

void cd(char * dir);
// EFFECTS: same as "cd" in linux command, change directory to dir.




#endif //VE482P1_BUILTIN_H
