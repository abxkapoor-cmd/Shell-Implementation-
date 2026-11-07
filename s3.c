#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);
    fflush(stdout); // print prompt now so the child doesn't print it again

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage,
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }

    args[*argsc] = NULL; ///args must be null terminated
}

// ---------- Redirection ----------

// True if the line has a < or >
int command_with_redirection(char line[])
{
    return strchr(line, '<') != NULL || strchr(line, '>') != NULL;
}

// Returns index of the operator (-1 if none), sets *type to which one
// e.g. "sort > out.txt" -> returns 1, type = REDIR_OUT
int find_redirection(char *args[], int argsc, enum RedirType *type)
{
    for (int i = 0; i < argsc; i++)
    {
        if (strcmp(args[i], "<") == 0)  { *type = REDIR_IN;     return i; }
        if (strcmp(args[i], ">") == 0)  { *type = REDIR_OUT;    return i; }
        if (strcmp(args[i], ">>") == 0) { *type = REDIR_APPEND; return i; }
    }
    *type = REDIR_NONE;
    return -1;
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.

    execvp(args[ARG_PROGNAME],args);
    perror(args[ARG_PROGNAME]);
    exit(1);

}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.

    if (args[0] == NULL){
        return;
    }
    if (strcmp(args[0],"exit") == 0){
        exit(0);
    }


    int rc = fork();
    if (rc<0) {
        perror("fork failed");
        return; // don't kill the shell if fork fails
    }
    else if (rc==0){
        child(args, argsc);
    }
    else{
        // main will reap
    }
}


void child_with_redirection(char *args[], int argsc, enum RedirType type, char *filename)
{
    int fd;
    if (type == REDIR_IN)
        fd = open(filename, O_RDONLY);                            // <
    else if (type == REDIR_OUT)
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);  // >  overwrite
    else
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644); // >> append

    if (fd < 0)
    {
        perror(filename);
        exit(1);
    }

    int slot = (type == REDIR_IN) ? STDIN_FILENO : STDOUT_FILENO;
    dup2(fd, slot); // slot now goes to the file
    close(fd);      // don't need the extra fd anymore

    child(args, argsc); // exec keeps the redirected slots
}

void launch_program_with_redirection(char *args[], int argsc)
{
    enum RedirType type;
    int i = find_redirection(args, argsc, &type);

    if (i <= 0 || i + 1 >= argsc) // need a command before and a file after
    {
        fprintf(stderr, "s3: syntax error near redirection\n");
        return;
    }

    char *filename = args[i + 1];
    args[i] = NULL; // cut args at the operator so the command doesn't see "> file"
    argsc = i;

    int rc = fork();
    if (rc < 0)
    {
        perror("fork failed");
        return;
    }
    else if (rc == 0)
    {
        child_with_redirection(args, argsc, type, filename);
    }
    // main will reap
}
