#ifndef _S3_H_
#define _S3_H_

///See reference for what these libraries provide
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

///Constants for array sizes, defined for clarity and code readability
#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PROMPT_LEN 256

///Enum for readable argument indices (use where required)
enum ArgIndex
{
    ARG_PROGNAME,
    ARG_1,
    ARG_2,
    ARG_3,
};

enum RedirType
{
    REDIR_NONE,   /// no operator
    REDIR_IN,     /// <   read stdin from a file
    REDIR_OUT,    /// >   write stdout to a file (overwrite)
    REDIR_APPEND, /// >>  write stdout to a file (append)
};

///With inline functions, the compiler replaces the function call
///with the actual function code;
///inline improves speed and readability; meant for short functions (a few lines).
///the static here avoids linker errors from multiple definitions (needed with inline).
static inline void reap()
{
    wait(NULL);
}

///Shell I/O and related functions (add more as appropriate)
int read_command_line(char line[]);
void construct_shell_prompt(char shell_prompt[]);
int parse_command(char line[], char *args[], int *argsc);

// redirection parsing
int command_with_redirection(char line[]);
void launch_program_with_redirection(char *args[], int argsc);
void child_with_redirection(char *args[], int argsc, enum RedirType type, char *filename);
int find_redirection(char *args[], int argsc, enum RedirType *type);


///Child functions (add more as appropriate)
void child(char *args[], int argsc);

///Program launching functions (add more as appropriate)
void launch_program(char *args[], int argsc);
int command_with_pipe(char *line);
void launch_pipeline(char *line);
void run_batch(char *line);
#endif
