#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pish.h"

static char pish_history_path[1024] = {'\0'};
/*
 * Set history file path to ~/.pish_history.
 */
static void set_history_path()
{
    const char *home = getpwuid(getuid())->pw_dir;
    strncpy(pish_history_path, home, 1024);
    strcat(pish_history_path, "/.pish_history");
}

void add_history(const struct pish_arg *arg)
{
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }
    /* 
     * TODO:
     * - open (and create if needed) history file at pish_history_path.
     * - write out the command stored in `arg`; argv values are separated
     *   by a space.
     */
    // user enters an empty command 0 length or whitespace
    int i ;
    int empty = 1;
    for (int i = 0; i < arg->argc; i++) {
        for (char *c = arg->argv[i]; *c!= '\0'; c++) {
                if (*c != ' ' && *c != '\t' && *c != '\n' && *c!= '\r') {
                    empty = 0;
                    break;
            }
        }
        if (empty) break;
    }

    
    if(empty) {
        return;
    }

     FILE *history = fopen(pish_history_path,"a"); //append 
     if(!history){
        perror("open");
        return EXIT_FAILURE;
     }
     int j;
     for (j =0; j < arg->argc; j++){
        fprintf(history,"%s",arg->argv[j]);
        if(j < arg->argc -1){
            fprintf(history, " ");
        }
     }   
    fclose(history);
}

void print_history()
{
    // set history path if needed
    if (!(*pish_history_path)) {
        set_history_path();
    }

    /* TODO: read history file and print with index */
    FILE *history_file = fopen(pish_history_path,"r");
    if(history_file == NULL){
        perror("open");
        return EXIT_FAILURE;
    }
    char buf[1024];
    int index = 0;
    while(fgets(buf,sizeof(buf),history_file)){
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        printf("%d: %s\n", index++, buf);
    }
    fclose(history_file);
}
