#include "s3.h"

void construct_shell_prompt(char shell_prompt[])
{
    char directory[MAX_PROMPT_LEN];

    if (getcwd(directory, sizeof(directory)) == NULL) {
        strcpy(shell_prompt, "[s3]$ ");
        return;
    }

    snprintf(shell_prompt, MAX_PROMPT_LEN,
             "[%s s3]$ ", directory);
}

///Prints a shell prompt and reads input from the user
int read_command_line(char line[])
{
    if (isatty(STDIN_FILENO)) {
        char shell_prompt[MAX_PROMPT_LEN];
        construct_shell_prompt(shell_prompt);
        printf("%s", shell_prompt);
        fflush(stdout);
    }
    if (!fgets(line, MAX_LINE, stdin)) {
        if (ferror(stdin))
            perror("s3: input");
        return 0;
    }
    size_t n = strlen(line);
    if (n && line[n - 1] == '\n') {
        line[n - 1] = '\0';
    } else if (!feof(stdin)) {
        int c = getchar();
        if (c != '\n' && c != EOF) {
            while ((c = getchar()) != '\n' && c != EOF) {}
            fprintf(stderr, "s3: command line too long\n");
            line[0] = '\0';
        }
    }
    return 1;
}

/* Token text is packed back into line. Operator tokens use stable addresses
 * so a quoted or escaped ">" remains an ordinary argument. */
static char input_operator[] = "<";
static char output_operator[] = ">";
static char append_operator[] = ">>";

int parse_command(char line[], char *args[], int *argsc)
{
    char source[MAX_LINE];
    strcpy(source, line);
    char *read = source;
    char *write = line;
    *argsc = 0;
    while (*read) {
        while (*read == ' ' || *read == '\t' || *read == '\r') read++;
        if (!*read) break;
        if (*argsc == MAX_ARGS - 1) {
            fprintf(stderr, "s3: too many arguments\n");
            return 0;
        }
        if (*read == '<' || *read == '>') {
            char op = *read++;
            if (op == '<') args[(*argsc)++] = input_operator;
            else if (*read == '>') {
                read++;
                args[(*argsc)++] = append_operator;
            } else args[(*argsc)++] = output_operator;
            continue;
        }
        args[(*argsc)++] = write;
        char quote = 0;
        while (*read) {
            char c = *read;
            if (!quote && (c == ' ' || c == '\t' || c == '\r' || c == '<' || c == '>')) break;
            read++;
            if (!quote && (c == '\'' || c == '"')) quote = c;
            else if (quote && c == quote) quote = 0;
            else if (c == '\\' && quote != '\'') {
                if (!*read) {
                    fprintf(stderr, "s3: unfinished escape\n");
                    return 0;
                }
                /* In double quotes, only shell-special characters lose the slash. */
                if (quote == '"' && *read != '"' && *read != '\\' && *read != '$' && *read != '`')
                    *write++ = c;
                *write++ = *read++;
            } else *write++ = c;
        }
        if (quote) {
            fprintf(stderr, "s3: unmatched quote\n");
            return 0;
        }
        *write++ = '\0';
    }
    args[*argsc] = NULL;
    return 1;
}

// ---------- Redirection ----------

// True if the line has a < or >
int command_with_redirection(char line[])
{
    char quote = 0;
    for (const char *p = line; *p; p++) {
        if (*p == '\\' && quote != '\'' && p[1]) { p++; continue; }
        if (quote) { if (*p == quote) quote = 0; }
        else if (*p == '\'' || *p == '"') quote = *p;
        else if (*p == '<' || *p == '>') return 1;
    }
    return 0;
}

// Returns index of the operator (-1 if none), sets *type to which one
// e.g. "sort > out.txt" -> returns 1, type = REDIR_OUT
int find_redirection(char *args[], int argsc, enum RedirType *type)
{
    for (int i = 0; i < argsc; i++)
    {
        if (args[i] == input_operator)  { *type = REDIR_IN;     return i; }
        if (args[i] == output_operator)  { *type = REDIR_OUT;    return i; }
        if (args[i] == append_operator) { *type = REDIR_APPEND; return i; }
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

    (void)argsc;
    execvp(args[ARG_PROGNAME],args);
    perror(args[ARG_PROGNAME]);
    exit(1);

}

void launch_program(char *args[], int argsc)
{
    if (args[0] == NULL){
        return;
    }
    if (strcmp(args[0],"exit") == 0){
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        static char previous_directory[MAX_LINE] = "";
        char current_directory[MAX_LINE];

        if (argsc > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            return;
        }

        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("cd: getcwd");
            return;
        }

        char *directory;

        if (argsc == 1) {
            directory = getenv("HOME");
            if (directory == NULL) {
                fprintf(stderr, "cd: HOME is not set\n");
                return;
            }
        } else if (strcmp(args[1], "-") == 0) {
            if (previous_directory[0] == '\0') {
                fprintf(stderr, "cd: no previous directory\n");
                return;
            }
            directory = previous_directory;
        } else {
            directory = args[1];
        }

        if (chdir(directory) == -1) {
            perror("cd");
        } else {
            // Save the old path only if the change succeeded.
            strcpy(previous_directory, current_directory);
        }

        return;
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
        reap();
    }
}

static char *find_pipe(char *line)
{
    char quote = 0;
    int depth = 0;

    for (char *p = line; *p != '\0'; p++) {
        if (*p == '\\' && quote != '\'' && p[1] != '\0') {
            p++;
            continue;
        }

        if (quote != 0) {
            if (*p == quote) {
                quote = 0;
            }
            continue;
        }

        if (*p == '\'' || *p == '"') {
            quote = *p;
        } else if (*p == '(') {
            depth++;
        } else if (*p == ')') {
            depth--;
        } else if (*p == '|' && depth == 0) {
            return p;
        }
    }

    return NULL;
}

void launch_pipeline(char *line)
{
    char *separator = find_pipe(line);
    
    if (separator == NULL) {
        return;
    }
    
    *separator = '\0';
    
    char *left = line;
    char *right = separator + 1;
    
    char left_line[2 * MAX_LINE];
    char right_line[2 * MAX_LINE];
    
    strcpy(left_line, left);
    strcpy(right_line, right);
    
    char *left_args[MAX_ARGS];
    char *right_args[MAX_ARGS];
    int left_argsc;
    int right_argsc;
    
    if (!parse_command(left_line, left_args, &left_argsc)) {
        return;
    }
    
    if (!parse_command(right_line, right_args, &right_argsc)) {
        return;
    }
    
    if (left_argsc == 0 || right_argsc == 0) {
        fprintf(stderr, "s3: missing command beside pipe\n");
        return;
    }
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t left_pid = fork();

    if (left_pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    
    if (left_pid == 0) {
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }
    
        close(pipefd[0]);
        close(pipefd[1]);
    
        child(left_args, left_argsc);
    }

    pid_t right_pid = fork();

    if (right_pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(left_pid, NULL, 0);
        return;
    }
    
    if (right_pid == 0) {
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }
    
        close(pipefd[0]);
        close(pipefd[1]);
    
        child(right_args, right_argsc);
    }
    
    // Parent: close its copies of both ends.
    close(pipefd[0]);
    close(pipefd[1]);
    
    // Both children have started. Now wait for them.
    waitpid(left_pid, NULL, 0);
    waitpid(right_pid, NULL, 0);
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
    if (dup2(fd, slot) < 0) {
        perror("dup2");
        close(fd);
        exit(1);
    }
    close(fd);      // don't need the extra fd anymore

    child(args, argsc); // exec keeps the redirected slots
}

void launch_program_with_redirection(char *args[], int argsc)
{
    enum RedirType type;
    int i = find_redirection(args, argsc, &type);

    if (i < 0) {
        launch_program(args, argsc);
        return;
    }
    /* This stage supports one redirection per command. Reject extras rather
     * than silently executing a different command. */
    if (i == 0 || i + 1 >= argsc ||
        args[i + 1] == input_operator || args[i + 1] == output_operator ||
        args[i + 1] == append_operator) {
        fprintf(stderr, "s3: syntax error near redirection\n");
        return;
    }
    for (int j = i + 2; j < argsc; j++) {
        if (args[j] == input_operator || args[j] == output_operator || args[j] == append_operator) {
            fprintf(stderr, "s3: only one redirection is supported\n");
            return;
        }
    }
    char *filename = args[i + 1];
    /* Keep ordinary arguments following the filename. */
    for (int j = i; j + 2 <= argsc; j++) args[j] = args[j + 2];
    argsc -= 2;

    int rc = fork();
    if (rc < 0) {
        perror("fork failed");
        return;
    }
    if (rc == 0)
        child_with_redirection(args, argsc, type, filename);
    else
        reap();
}
