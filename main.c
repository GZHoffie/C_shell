#include <stdio.h>
#include  <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include  <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "builtin.h"


#define MAXSIZE 4096
#define DELIM " "

char * BACKGROUND_CHILD[MAXSIZE];
pid_t BACKGROUND_PID[MAXSIZE];
int BACKGROUND_NUMBER = 1;
int BACKGOUND = 0;



typedef enum redirection_options{
    OVERWRITE,
    APPEND,
    INPUT
} Redirect_Option_t;

int main();
int mumsh_main_loop();
char * mumsh_getl();
char ** mumsh_getargs(char * line);
int mumsh_parse(char ** args);
int mumsh_parse_line(char * line);
int mumsh_scan_args(char ** args);
int mumsh_redirection(char ** second, Redirect_Option_t option);
void mumsh_pipeline(char ** args);
void mumsh_pipeline_exec(char *** pipelines, int pipes_count);
void mumsh_get_arg_from_pipe(char *** pipelines, int pipes_count, char * arg);
void mumsh_jobs();

static void intHandler_main(int signum){
    printf("\nmumsh $ ");
    fflush(stdout);
}

static void intHandler(int signum){
    printf("\n");
    fflush(stdout);
}

static void chldHandler(int signum){
    while (waitpid(-1, 0, WNOHANG) > 0){}
}



int main(){
    signal(SIGINT, intHandler_main);
    mumsh_main_loop();
    //free(BACKGROUND_CHILD);
    for (int i = 1; i < BACKGROUND_NUMBER; i++){
        free(BACKGROUND_CHILD[i]);
    }
}




int mumsh_main_loop() {


    signal(SIGINT, intHandler_main);
    signal(SIGCHLD, chldHandler);
    //signal(SIGCHLD, SIG_IGN);
    char * line;
    char ** args;
    BACKGOUND = 0;
    line = mumsh_getl();
    args = mumsh_getargs(line);

    while (args == NULL || strcmp(args[0], "exit") != 0){
        //BACKGOUND = 0;
        if (args == NULL){
            line = mumsh_getl();
            args = mumsh_getargs(line);
            continue;
        }else if (strcmp(args[0], "cd") == 0){
            cd(args[1]);
        }else{
            mumsh_pipeline(args);
        }
        signal(SIGINT, intHandler_main);
        BACKGOUND = 0;
        line = mumsh_getl();
        args = mumsh_getargs(line);

    }
    printf("exit\n");
    fflush(stdout);



    return 0;
}

char * mumsh_getl(){
    // EFFECTS: print "mumsh $ " and then read and return the line entered by user.
    static char line[MAXSIZE];
    int temp;
    int number = 0;
    int flag = 0;
    printf("mumsh $ ");
    fflush(stdout);
    while (1) {
        if (flag == 0)
            temp = getchar();
        flag = 0;
        if (temp == -1)
            return "exit";
        if (temp == '\'' || temp == '\"') {
            char temp_next = (char)getchar();
            while (temp_next != temp){//read char until next quotation mark appears
                if (temp_next == '\n'){
                    line[number] = '\0';
                    if (mumsh_parse_line(line) == -1)
                        return NULL;
                    printf("> ");
                    fflush(stdout);
                }
                line[number] = temp_next;
                number++;
                temp_next = (char)getchar();
            }
        } else if (temp == 10){ //'\n'
            if (number == 0)
                return NULL;
            break;
        } else if (temp == '<' || temp == '|') {
            line[number] = ' ';
            line[number + 1] = '$'; //indicating that this is redirection/pipe instead of a simple char.
            line[number + 2] = (char)temp;
            line[number + 3] = ' ';
            number+=4;
            char temp_next = (char)getchar();
            while (temp_next == '\n'){
                line[number] = '\0';
                if (mumsh_parse_line(line) == -1)
                    return NULL;
                printf("> ");
                fflush(stdout);
                temp_next = (char)getchar();
            }
            temp = temp_next;
            flag = 1;
            continue;
        } else if (temp == '>'){
            char temp_next = (char)getchar();
            line[number] = ' ';
            line[number + 1] = '$'; //indicating that this is redirection/pipe instead of a simple char.
            line[number + 2] = (char)temp;
            if (temp_next == '>'){
                line[number + 3] = temp_next;
                line[number + 4] = ' ';
                temp_next = (char)getchar();
                while (temp_next == '\n'){
                    line[number + 4] = '\0';
                    if (mumsh_parse_line(line) == -1)
                        return NULL;
                    line[number + 4] = ' ';
                    printf("> ");
                    fflush(stdout);
                    temp_next = (char)getchar();
                }
            }
            else{
                line[number + 3] = ' ';
                line[number + 4] = ' ';
                while (temp_next == '\n'){
                    line[number + 4] = '\0';
                    if (mumsh_parse_line(line) == -1)
                        return NULL;
                    line[number + 4] = ' ';
                    printf("> ");
                    fflush(stdout);
                    temp_next = (char)getchar();
                }
            }
            temp = temp_next;
            flag = 1;
            number+=5;

        } else{
            line[number] = (char)temp;
            number++;
        }
    }
    line[number] = '\0';
    if (mumsh_parse_line(line) == -1)
        return NULL;
    else if (mumsh_parse_line(line) == 1) {
        line[number - 1] = '\0';
        BACKGOUND = 1; // the process will be run in background
    }
    return line;
}

int mumsh_parse_line(char * line){
    char * line_cpy = (char *)malloc((strlen(line) + 1)*sizeof(char));
    strcpy(line_cpy, line);
    char ** args = mumsh_getargs(line_cpy);
    int syn = mumsh_parse(args);
    if (syn == 0){
        char temp = line_cpy[strlen(line) - 1];
        free(line_cpy);
        return (temp == '&');
    }
    free(line_cpy);
    return syn;
}

char ** mumsh_getargs(char * line){
    // EFFECTS: split line into several arguments
    if (line == NULL)
        return NULL;


    static char * args[MAXSIZE];
    int number = 0;
    char * delim = DELIM; // delimiters of command
    char * temp = NULL;
    temp = strtok(line, delim);
    while (temp != NULL){
        args[number] = temp;
        number++;
        temp = strtok(NULL, delim);
    }
    args[number] = NULL;
    return args;
}

int mumsh_parse(char ** args){
    // EFFECTS: return whether the arguments will be run in background, and judge
    //          syntax/miss program error.
    if (args == NULL)
        return 0;
    else{
        int number = 0;
        while (args[number + 1] != NULL){
            if (strcmp(args[number], "$|") == 0){
                if (number == 0 || strcmp(args[number + 1], "$|") == 0){
                    printf("error: missing program\n");
                    return -1;
                }
                int temp = 2;
                while (args[number + temp] != NULL){
                    if (strcmp(args[number + temp], "$<") == 0){
                        printf("error: duplicated input redirection\n");
                        return -1;
                    }
                    temp++;
                }
            }
            if (strcmp(args[number], "$>") == 0
            ||  strcmp(args[number], "$>>") == 0){
                if (number == 0 || strcmp(args[number - 1], "$|") == 0){
                    if (args[number + 2] != NULL && strcmp(args[number + 2], "$|") == 0){
                        printf("error: missing program\n");
                        return -1;
                    }
                }
                if (strcmp(args[number + 1], "$>") == 0
                    ||  strcmp(args[number + 1], "$>>") == 0
                    ||  strcmp(args[number + 1], "$<") == 0
                    ||  strcmp(args[number + 1], "$|") == 0){
                    printf("syntax error near unexpected token `%s\'\n", &args[number + 1][1]);
                    return -1; // syntax error
                }
                int temp = 2;
                while (args[number + temp] != NULL){
                    if (strcmp(args[number + temp], "$>") == 0
                        ||  strcmp(args[number + temp], "$>>") == 0
                        ||  strcmp(args[number + temp], "$|") == 0){
                        printf("error: duplicated output redirection\n");
                        return -1;
                    }
                    temp++;
                }
            }
            if (strcmp(args[number], "$<") == 0){
                if (number == 0 || strcmp(args[number - 1], "$|") == 0){
                    if (args[number + 2] != NULL && strcmp(args[number + 2], "$|") == 0){
                        printf("error: missing program\n");
                        return -1;
                    }
                }
                if (strcmp(args[number + 1], "$>") == 0
                    ||  strcmp(args[number + 1], "$>>") == 0
                    ||  strcmp(args[number + 1], "$<") == 0
                    ||  strcmp(args[number + 1], "$|") == 0){
                    printf("syntax error near unexpected token `%s\'\n", &args[number + 1][1]);
                    return -1; // syntax error
                }
                int temp = 2;
                while (args[number + temp] != NULL){
                    if (strcmp(args[number + temp], "$<") == 0){
                        printf("error: duplicated input redirection\n");
                        return -1;
                    }
                    temp++;
                }
            }
            number++;
        }

    }
    return 0;

}


int mumsh_scan_args(char ** args){
    // EFFECTS: scan through the argument and split according to the redirection symbols
    //signal(SIGINT, SIG_DFL);

    fflush(stdout); // make sure the buffer is empty.
    // storing file descriptor
    int stdout_copy = dup(1);
    int stdin_copy = dup(0);
    int file[10];
    int file_count = 0;
    int built_in = 0;
    if (args == NULL) {
        printf("error: missing argments\n");
        exit(1);
    }
    int number = 0;
    char ** next_ptr = NULL;
    while (args[number] != NULL){ // get the total number of commands
        int flag = -1;

        if (strcmp(args[number], "$>") == 0)
            flag = 0;
        else if (strcmp(args[number], "$>>") == 0)
            flag = 1;
        else if (strcmp(args[number], "$<") == 0)
            flag = 2;
        if (flag >= 0){
            next_ptr = args + number + 1;
            file[file_count] = mumsh_redirection(next_ptr, (Redirect_Option_t)flag);
            file_count++;
            int temp = number;
            while (args[number] != NULL){
                args[number] = args[number + 2];
                number++;
            }
            number = temp - 1;
        }
        number++;
    }
    //mumsh_exec(args);
    if (args[0] == NULL) {
        printf("error: missing program\n");
        exit(1);
    }else if (strcmp(args[0], "cd") == 0){
        cd(args[1]);
        built_in = 1;
    }else if (strcmp(args[0], "jobs") == 0){
        mumsh_jobs();
        built_in = 1;
    }else {
        built_in = mumsh_builtin(args);
    }
    if (built_in != 1) {
        //fprintf(stderr,"%s\n", *args);
        if (execvp(args[0], args) < 0) {
            printf("%s: command not found\n", args[0]);
            exit(1);
        }
    }
    for (int i = 0; i < file_count; i++){
        close(file[file_count]);
    }

    dup2(stdout_copy, 1); // redirect output
    dup2(stdin_copy, 0);
    close(stdin_copy);
    close(stdout_copy);
    //fflush(stdout);
    return built_in;
}

int mumsh_redirection(char ** second, Redirect_Option_t option){


    int file;
    int flag;
    //printf("before dup2: %d\n",STDOUT_FILENO);
    switch (option){
        case OVERWRITE:
            file = open(second[0], O_CREAT|O_WRONLY|O_TRUNC, 0666);
            flag = dup2(file, 1);

            if (file < 0){
                printf("%s: Permission denied\n", second[0]);
                close(file);
                exit(1);
            }
            break;
        case APPEND:
            file = open(second[0], O_APPEND|O_WRONLY|O_CREAT, 0666);
            flag = dup2(file, 1);
            if (file < 0){
                printf("%s: Permission denied\n", second[0]);
                close(file);
                exit(1);
            }
            break;
        default: // redirect input
            file = open(second[0], O_RDONLY, 0666);
            flag = dup2(file, 0);
            if (file < 0){
                printf("%s: No such file or directory\n", second[0]);
                close(file);
                exit(1);
            }
    }
    //fprintf(stderr, "after dup2: %d\n",STDOUT_FILENO);
    //fflush(stderr);

    if (flag < 0){
        printf("unable to duplicate.\n");
        close(file);
        exit(1);
    }

    return file;

}


void mumsh_pipeline(char ** args){
    // EFFECTS: scan through args and split into pipelines, then execute
    if (args == NULL)
        return;
    int number = 0;
    int pipes_count = 0;
    char ** pipelines[MAXSIZE];
    pipelines[0] = args;

    // get the total number of arguments
    while (args[number] != NULL){
        if (strcmp(args[number], "$|") == 0){
            args[number] = NULL;
            pipes_count++;
            pipelines[pipes_count] = &args[number+1];
        }
        number++;
    }



    pipelines[pipes_count+1] = NULL;
    mumsh_pipeline_exec(pipelines, pipes_count);

}

void mumsh_pipeline_exec(char *** pipelines, int pipes_count){
    //int folders = pipes_count*2;
    int flag;
    int fd[pipes_count*2 + 1];
    int stat;
    pid_t pid[pipes_count+1];
    for (int i = 0; i < pipes_count; i++){
        //fprintf(stderr,"piping fd[%d] and fd[%d] together\n", i * 2, i * 2+1);
        if (pipe(fd + i * 2) < 0){
            perror("pipe: unable to create pipe");
            exit(1);
        }
    }

    for (int i = 0; i < pipes_count + 1; i++){
        //fprintf(stderr,"begin loop %d\n", i);
        pid[i] = fork();
        signal(SIGINT, intHandler);
        if (pid[i] == 0){
            if (i > 0){
                //fprintf(stderr,"duplicating fd[%d] to stdin\n", (i-1)*2);

                if (dup2(fd[(i-1)*2], 0) < 0) { // read form stdin, 0, 2, 4, ...
                    perror("dup2: unable to duplicate standard input");
                    exit(1);
                }
            }
            if (i < pipes_count){
                //fprintf(stderr,"duplicating fd[%d] to stdout\n", i*2+1);
                if (dup2(fd[i*2+1], 1) < 0) { // write to 1, 3, 5, 7, ..., stdout
                    perror("dup2: unable to duplicate standard output");
                    exit(1);
                }
            }
            for (int j = 0; j < pipes_count*2; j++){
                close(fd[j]);
            }
            //fprintf(stderr,"step %d, execute %s\n", i,*pipelines[i]);

            flag = mumsh_scan_args(pipelines[i]);

            if (flag != 1){

                exit(1);
            }else{
                exit(0);
            }



        } else if (pid[i] == -1){
            perror("fork: unable to create child process");
            exit(1);
        }
        //wait(&pid);

    }
    for (int i = 0; i < pipes_count*2; i++){
        close(fd[i]);
    }
    if (BACKGOUND == 0) {
        for (int i = 0; i < pipes_count + 1; i++) {
            waitpid(pid[i], &stat, 0);
            //fprintf(stderr, "process %d completed\n", i);
        }
    }else{
        BACKGROUND_CHILD[0] = "";
        char arg[MAXSIZE] = "";
        mumsh_get_arg_from_pipe(pipelines, pipes_count, arg);
        BACKGROUND_CHILD[BACKGROUND_NUMBER] = (char *)malloc((strlen(arg) + 1)*sizeof(char));
        strcpy(BACKGROUND_CHILD[BACKGROUND_NUMBER], arg);
        printf("[%d] %s\n", BACKGROUND_NUMBER, BACKGROUND_CHILD[BACKGROUND_NUMBER]);
        BACKGROUND_PID[BACKGROUND_NUMBER] = pid[pipes_count];
        //printf("%d", pid[pipes_count]);
        BACKGROUND_NUMBER++;


    }

    //wait(&stat);
    //fflush(stdout);

}


void mumsh_jobs(){

    for (int i = 1; i < BACKGROUND_NUMBER; i++){

        int stat;

        //printf("%d",BACKGROUND_PID[i]);
        //pid_t pid = waitpid(BACKGROUND_PID[i], &stat, WNOHANG);

        pid_t pid = waitpid(BACKGROUND_PID[i], &stat, 0);

        if (getpgid(BACKGROUND_PID[i]) >= 0)
            printf("[%d] running %s\n", i, BACKGROUND_CHILD[i]);
        else{
            printf("[%d] done %s\n", i, BACKGROUND_CHILD[i]);
        }


        /*
        if (kill(BACKGROUND_PID[i], 0) == 0)
            printf("[%d] done %s\n", i, BACKGROUND_CHILD[i]);
        else
            printf("[%d] running %s\n", i, BACKGROUND_CHILD[i]);
            */


        fflush(stdout);

    }
}

void mumsh_get_arg_from_pipe(char *** pipelines, int pipes_count, char * arg){

    for (int i = 0; i < pipes_count + 1; i++){
        int j = 0;
        while (pipelines[i][j] != NULL){
            if (strcmp(pipelines[i][j], "$>>")==0)
                strcat(arg, ">> ");
            else if (strcmp(pipelines[i][j], "$>")==0)
                strcat(arg, "> ");
            else if (strcmp(pipelines[i][j], "$<")==0)
                strcat(arg, "< ");
            else{
                strcat(arg, pipelines[i][j]);
                strcat(arg, " ");
            }
            j++;
        }
        if (i != pipes_count)
            strcat(arg, "| ");

    }
    strcat(arg, "&\0");

}