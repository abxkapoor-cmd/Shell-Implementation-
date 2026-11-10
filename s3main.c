#include "s3.h"

int main(void)
{
    /* Extra room for terminators inserted around adjacent operators. */
    char line[2 * MAX_LINE];
    char *args[MAX_ARGS];
    int argsc;

    while (read_command_line(line)) {
        int redirected = command_with_redirection(line);
        if (!parse_command(line, args, &argsc) || argsc == 0)
            continue;
        if (redirected)
            launch_program_with_redirection(args, argsc);
        else
            launch_program(args, argsc);
    }
    return ferror(stdin) ? 1 : 0;
}
