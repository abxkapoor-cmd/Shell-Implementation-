#include "s3.h"

int main(int argc, char *argv[])
{
    /* Extra room for terminators inserted around adjacent operators. */
    char line[2 * MAX_LINE];
    
    if (argc>1){
    run_batch(argv[1]);
    return 0;
    }

    while (read_command_line(line)) {
        run_batch(line);
    }

    return ferror(stdin) ? 1 : 0;

}

