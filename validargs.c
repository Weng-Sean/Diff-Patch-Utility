#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    // printf("%d\n", argc);
    // printf("%s\n", *argv);

    if (argc <= 1){
        // printf("MISSING ARGS");
        return -1;
    }

    int nFlag = 0, qFlag = 0;

    argv++;
    argc--;

    int argCount = 0;
    int fileExist = 0;

    while (1){
        // printf("%s\n", *argv);
        argCount++;
        if (**argv != '-'){
            diff_filename = *argv;
            // printf("diff_filename: %s\n", *argv);
            argc--;
            fileExist = 1;
            break;
        }
        else{
            (*argv)++;
            // detect invalid command. ex. -q-n, -qq, -nn
            if(*(*argv + 1) != 0){
                // printf("INVALID COMMAND LENGTH");
                return -1;
            }
            switch(**argv){
                case 'h':
                    // printf("h detected\n");
                    // return -1 if -h is not the first arg
                    if (argCount != 1){
                        // printf("INVALID -h POSITION");
                        return -1;
                    }
                    global_options += 1;
                    // printf("global_options: %ld\n", global_options);
                    return 0;
                case 'n':
                    // printf("n detected\n");
                    if (!nFlag)
                        global_options += 2;
                    nFlag = 1;
                    break;
                case 'q':
                    // printf("q detected\n");
                    if (!qFlag)
                        global_options += 4;
                    qFlag = 1;
                    break;
                default:
                    // printf("INVALID FLAG");
                    return -1;
            }
        }
        argc--;

        if (argc > 0)
            argv++;
        else
            break;

    }


    if(argc != 0){
        // printf("INVALID FILENAME POSITION");
        return -1;
    }

    if(!fileExist){
        // printf("DIFF_FILE IS MISSING");
        return -1;
    }

    // printf("global_options: %ld\n", global_options);
    return 0;
}

