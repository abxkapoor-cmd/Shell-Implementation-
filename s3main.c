#include "s3.h"

int main(void)
{
    /* Extra room for terminators inserted around adjacent operators. */
    char line[2 * MAX_LINE];


    while (read_command_line(line)) {
        run_batch(line);
    }

    return ferror(stdin) ? 1 : 0;

}

