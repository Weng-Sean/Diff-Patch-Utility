#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"


/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */


static void clear_buffer(){
    for (int i = 0; i < 512; i++){
        *(hunk_deletions_buffer + i) = 0;
        *(hunk_additions_buffer + i) = 0;
    }
}

static int isDigit(char c) {
    return c >= '0' && c <= '9';
}


// return a number or -1 if invalid
static int get_num(FILE *in) {
    int num = 0;
    char c;
    int isValid = 0;
    while(1){
        c = fgetc(in);
        // printf("get_num: %c\n", c);
        if (!isDigit(c)){
            ungetc(c, in);
            // printf("not a digit\n");
            break;
        }
        num = num * 10 + (c - '0');
        isValid = 1;
    };
    if (!isValid){
        if (c - EOF == 0){
            return EOF;
        }
        return ERR;
    };
    return num;
}


// if hunk_next_flag = 1 means hunk_next() function was just called
static int hunk_next_flag_getc_helper = 0;
static int hunk_next_flag_getc = 0;
static int BOF = 1;
static int change_type_flag = 0; // if current hunk has a change type then the flag is 1, otherwise it is 0.

int hunk_next(HUNK *hp, FILE *in) {

    // printf("enter hunk next function body\n");
    // update the hunk_next_flag for looping
    if (BOF){
        hunk_next_flag_getc_helper = 1;
        hunk_next_flag_getc = 1;
        BOF = 0;
    }

    // loop to next hunk head
    char c = hunk_getc(hp, in);
    // printf("char c: %c, %d\n", c, c);
    while (c - ERR != 0){
        // printf("looping:\t\t%c\t\t%d\n", c, c);
        c = hunk_getc(hp, in);
    }

    // printf("\n\n\nHUNK SHOW:\n");
    // hunk_show(hp, stdout);
    clear_buffer();

    // update the hunk_next_flag for the rest of the program
    hunk_next_flag_getc_helper = 1;
    hunk_next_flag_getc = 1;


    // parse the head
    (*hp).serial ++;
    // old start
    int num = get_num(in);
    if (num == ERR){
        // printf("hunk_next ERR1\n");
        return ERR;
    }
    if (num == EOF){
        // printf("hunk_next EOF1\n");
        return EOF;
    }
    (*hp).old_start = num;

    // old end
    c = fgetc(in);
    if (c != ','){
        ungetc(c, in);
        (*hp).old_end = num;
    }
    else{
        num = get_num(in);
        if (num == ERR){
            // printf("hunk_next ERR2\n");
            return ERR;
        }
        if (num == EOF){
            // printf("hunk_next EOF2\n");
            return EOF;
        }
        (*hp).old_end = num;
    }

    // type
    c = fgetc(in);
    // printf("type: %c\n", c);
    switch(c){
        case 'a':
            (*hp).type = HUNK_APPEND_TYPE;
            change_type_flag = 0;
            break;
        case 'd':
            (*hp).type = HUNK_DELETE_TYPE;
            change_type_flag = 0;
            break;
        case 'c':
            (*hp).type = HUNK_CHANGE_TYPE;
            change_type_flag = 1;
            break;
        default:
            if (c - EOF == 0){
                return EOF;
            }
            // printf("hunk_next ERR3\n");
            return ERR;
    }

    // new start
    num = get_num(in);
    if (num == ERR){
        // printf("hunk_next ERR4\n");
        return ERR;
    }
    if (num == EOF){
        // printf("hunk_next EOF4\n");
        return EOF;
    }
    (*hp).new_start = num;

    // new end
    c = fgetc(in);
    if (c != ','){
        ungetc(c, in);
        (*hp).new_end = num;
    }
    else{
        num = get_num(in);
        if (num == ERR){
            // printf("hunk_next ERR5\n");
           return ERR;
        }
        if (num == EOF){
            // printf("hunk_next EOF5\n");
            return EOF;
        }

        (*hp).new_end = num;
    }

    // check for new line
    c = fgetc(in);
    if (c != '\n'){
        // printf("hunk_next ERR6\n");
        return ERR;
    }




    // check for possible errors

    // 1. old_end <= old_start || new_end <= new_start
    if ((*hp).old_end < (*hp).old_start || (*hp).new_end < (*hp).new_start){
        // printf("hunk_next ERR7\n");
        return ERR;
    }
    return 0;
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

// beginning of a line flag
static int BOL = 0;
// CAT => Current Action Type ('a', 'd', 'n') => return EOS
static char CAT = 'n';
// check syntax error
static char expected_type = 'n';
static char last_hunk_getc_result;
static char last_hunk_getc_result_used = 0;
static int return_EOS = 0;

int hunk_getc_helper(HUNK *hp, FILE *in) {
    // if the hunk_next function was just being called
    if (hunk_next_flag_getc_helper){
        hunk_next_flag_getc_helper = 0;
        BOL = 1;
        expected_type = 'n';
        CAT = 'n';

    }

    if (expected_type == 'n'){
        switch((*hp).type){
        case HUNK_APPEND_TYPE:
            expected_type = 'a';
            break;
        case HUNK_DELETE_TYPE:
            expected_type = 'd';
            break;
        case HUNK_CHANGE_TYPE:
            expected_type = 'd';
            break;
        default:
            // printf("getc error 8\n");
            return ERR;
        }
    }

    // if BOL, then do pattern-matching
    if (BOL){
        // change the BOL flag
        BOL = 0;

        char c1 = fgetc(in);

        // printf("c1: %c\t\t%d\n", c1, c1);
        char c2 = fgetc(in);
        // printf("c2: %c\t\t%d\n", c2, c2);

        // match the first char in the current line
        switch(c1){
        case '<':
            // "< " pattern matched
            if (c2 == ' '){

                // check for syntax error
                if (expected_type != 'd'){
                    // printf("return 2\n");
                    return ERR;
                }

                // if current action type == 'n', then modify its value
                if (CAT == 'n'){
                    CAT = 'd';
                }
                // if current action type does not match, then return 'EOS'
                else if (CAT != 'd'){
                    CAT = 'd';
                    // printf("return 1\n");
                    return EOS;
                }

                // return the next char by calling hunk_getc recursively
                // printf("return 3\n");
                return hunk_getc_helper(hp, in);
            }
            // invalid pattern
            else{
                // push all the chars back to the file
                ungetc(c2, in);
                ungetc(c1, in);

                // printf("getc error 1\n");
                return ERR;
            }
        case '>':
             // check for syntax error
            if (expected_type != 'a'){
                // printf("getc error 5\n");
                return ERR;
            }

            // "> " pattern matched
            if (c2 == ' '){
                // printf("CAT: %c\n", CAT);
                // if current action type == 'n', then modify its value
                if (CAT == 'n'){
                    CAT = 'a';
                }
                // if current action type does not match, then return 'EOS'
                else if (CAT != 'a'){
                    CAT = 'a';
                    // printf("456\n");
                    return EOS;
                }


                // return the next char by calling hunk_getc recursively
                return hunk_getc_helper(hp, in);
            }
            // invalid pattern
            else{
                // push all the chars back to the file
                ungetc(c2, in);
                ungetc(c1, in);

                // printf("getc error 2\n");
                return ERR;
            }
        case '-':
            char c3 = fgetc(in);
            char c4 = fgetc(in);
            // "---\n" pattern matched
            if (c2 == '-' && c3 == '-' && c4 == '\n'){

                // if the action type of the current hunk is not C, then return an err
                if ((*hp).type != HUNK_CHANGE_TYPE){
                    return ERR;
                }
                // change expected type
                else{
                    expected_type = 'a';
                }
                BOL = 1;
                // return the next char by calling hunk_getc recursively
                return hunk_getc_helper(hp, in);
            }
            else{
                // push all the chars back to the file
                ungetc(c4, in);
                ungetc(c3, in);
                ungetc(c2, in);
                ungetc(c1, in);

                // printf("getc error 3\n");
                return ERR;
            }
        default:
            // implies error or end of the hunk
            // push all the chars back to the file
            ungetc(c2, in);
            ungetc(c1, in);

            // cannot proceed and thus turn the flag back on
            BOL = 1;

            if (CAT != 'n'){
                CAT = 'n';
                // printf("789\n");
                return EOS;
            }

            // printf("getc error 4\n");
            return ERR;
        }
    }
    // not BOL
    else{

        char c1 = fgetc(in);
        if (c1 - EOF == 0){
            // printf("getc error 7\n");
            if (last_hunk_getc_result != '\n' && last_hunk_getc_result_used == 0){
                last_hunk_getc_result_used = 1;
                last_hunk_getc_result = '\n';
                return_EOS = 1;
                return '\n';
            }
            if (return_EOS){
                return_EOS = 0;
                return EOS;
            }
            return ERR;
        }
        else if (c1 == '\n'){
            BOL = 1;
        }
        // printf("c1: %c\n", c1);
        return c1;
    }

}


static int new_line_flag = 1;
static char* hunk_deletions_buffer_iter = hunk_deletions_buffer;
static char* deletions_count_char = hunk_deletions_buffer;
static char* hunk_additions_buffer_iter = hunk_additions_buffer;
static char* additions_count_char = hunk_additions_buffer;
static int char_count = 0;


int hunk_getc(HUNK *hp, FILE *in) {
    char c = hunk_getc_helper(hp, in);
    // printf("c: %c, d: %d", c, c);


    if (hunk_next_flag_getc){
        hunk_deletions_buffer_iter = hunk_deletions_buffer;
        deletions_count_char = hunk_deletions_buffer;
        hunk_additions_buffer_iter = hunk_additions_buffer;
        additions_count_char = hunk_additions_buffer;

        *hunk_deletions_buffer_iter = 0;
        *(hunk_deletions_buffer_iter+1) = 0;
        *hunk_additions_buffer_iter = 0;
        *(hunk_additions_buffer_iter+1) = 0;

        char_count = 0;

        hunk_next_flag_getc = 0;
    }

    // skip EOS
    if (c - EOS == 0){
        last_hunk_getc_result = c;
        return c;
    }

    if (c - ERR == 0){
        last_hunk_getc_result = c;
        return ERR;
    }

    // add c to the buffer according to CAT
    // printf("CHAR: %c, %d, CAT: %c\n", c, c, CAT);
    switch(CAT){
        case 'a':
            if (new_line_flag){
                char_count = 0;
                additions_count_char = hunk_additions_buffer_iter;
                *additions_count_char = 0;
                hunk_additions_buffer_iter += 2;
                new_line_flag = 0;
                // printf("+2\n");
            }

            if (hunk_additions_buffer_iter - hunk_additions_buffer < 510){
                *(hunk_additions_buffer_iter++) = c;
                if (++char_count <= 255){
                    *additions_count_char = char_count;
                }
                else{
                    *additions_count_char = char_count % 256;
                    *(additions_count_char + 1) = char_count / 256;
                }
            }

            break;
        case 'd':
            if (new_line_flag){
                char_count = 0;
                deletions_count_char = hunk_deletions_buffer_iter;
                *deletions_count_char = 0;
                hunk_deletions_buffer_iter += 2;
                new_line_flag = 0;
                // printf("+2\n");
            }

            if (hunk_deletions_buffer_iter - hunk_deletions_buffer < 510){
                *(hunk_deletions_buffer_iter++) = c;

                if (++char_count <= 255){
                    *deletions_count_char = char_count;
                }
                else{
                    *deletions_count_char = char_count % 256;
                    *(deletions_count_char + 1) = char_count / 256;
                }
            }

            break;
    }

    // update the new line flag according to c
    if (c == '\n'){
        new_line_flag = 1;
    }

    // printf("--------------\n");
    // for (int i = 0; i < 100; i++){
    //         char c_ = *(hunk_deletions_buffer + i);
    //         if (c_ == '\n'){
    //             printf("idx:%d\t\tchar:\t\tnum val:%u\n", i, c_);
    //         }
    //         else{
    //             printf("idx:%d\t\tchar:%c\t\tnum val:%d\n", i, c_, c_);
    //         }
    //     }
    // hunk_show(hp, stdout);

    last_hunk_getc_result = c;
    return c;
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet beenstored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer,  made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    // header
    fprintf(out, "%d", (*hp).old_start);
    if((*hp).old_start != (*hp).old_end)
        fprintf(out, ",%d", (*hp).old_end);

    switch((*hp).type){
        case  HUNK_APPEND_TYPE:
            fprintf(out, "a");
            break;
        case HUNK_DELETE_TYPE:
            fprintf(out, "d");
            break;
        case HUNK_CHANGE_TYPE:
            fprintf(out, "c");
            break;
        default:
            break;
    }

    fprintf(out, "%d", (*hp).new_start);
    if((*hp).new_start != (*hp).new_end)
        fprintf(out, ",%d", (*hp).new_end);

    fprintf(out, "\n");

    // body content
    int new_line_d = 0;
    if (hp->type == HUNK_DELETE_TYPE || hp->type == HUNK_CHANGE_TYPE){
        fprintf(out, "< ");
        int line_char_count_d = (unsigned char) *hunk_deletions_buffer + *(hunk_deletions_buffer + 1) * 256;
        // fprintf(out, "\n\n%d\n\n", line_char_count_d);
        char *ptr_d = hunk_deletions_buffer + 2;

        while(line_char_count_d != 0){
            for (int i = 0; i < line_char_count_d; i++){

                if (new_line_d){
                    fprintf(out, "< ");
                }
                fprintf(out,"%c", *ptr_d);
                if (*ptr_d == '\n'){
                    new_line_d = 1;
                }
                else{
                    new_line_d = 0;
                }
                // printf("%c, %d \n", *ptr_d, *ptr_d);
                ptr_d++; // increment the ptr

            }

            line_char_count_d = (unsigned char) *ptr_d + *(ptr_d + 1) * 256;
            ptr_d += 2;

        }

        // overflow case
        if (ptr_d > hunk_deletions_buffer + 256){
            fprintf(out, "...\n");
        }
    }

    int change_type_separator = 0;
    int new_line_a = 0;
    if ((*hp).type == HUNK_CHANGE_TYPE){
        change_type_separator = 1;
    }

    if (hp->type == HUNK_APPEND_TYPE || hp->type == HUNK_CHANGE_TYPE){
        if (!change_type_separator){
            fprintf(out, "> ");

        }

        int line_char_count_a = (unsigned char) *hunk_additions_buffer + *(hunk_additions_buffer + 1) * 256;
        // fprintf(out, "\n\n%d\n\n", line_char_count_d);
        char *ptr_a = hunk_additions_buffer + 2;

        while(line_char_count_a != 0){
            if (change_type_separator){
                change_type_separator = 0;
                fprintf(out, "---\n> ");
            }
            for (int i = 0; i < line_char_count_a; i++){
                if (new_line_a){
                    fprintf(out, "> ");
                }
                fprintf(out,"%c", *ptr_a);
                // printf("\n%c,%d\n", *ptr_a, *ptr_a);
                if (*ptr_a == '\n'){
                    new_line_a = 1;
                }
                else{
                    new_line_a = 0;
                }
                // printf("%c, %d \n", *ptr_d, *ptr_d);
                ptr_a++; // increment the ptr

            }

            line_char_count_a = (unsigned char) *ptr_a + *(ptr_a + 1) * 256;
            ptr_a += 2;

        }

        // overflow case
        if (ptr_a > hunk_additions_buffer + 256){
            fprintf(out, "...\n");
        }
    }

    // int count_A = 0;
    // char *charCountPtr_A = hunk_additions_buffer;
    // int lineCharCount_A = (unsigned char) *charCountPtr_A + *(charCountPtr_A + 1) * 256;
    // char *currentPtr_A;
    // while(lineCharCount_A != 0){
    //     if (change_type_separator){
    //         change_type_separator = 0;
    //         fprintf(out, "---\n> ");
    //     }
    //     count_A += 2; // char count takes up two bytes
    //     // printf("lineCharCount_A: %d\n", lineCharCount_A);
    //     for (int i = 0; i < lineCharCount_A; i++){

    //         if(++count_A >= 512){
    //         fprintf(out, "...\n");
    //         break;
    //     }


    //         currentPtr_A = charCountPtr_A + i + 2;
    //         fprintf(out, "%c",*currentPtr_A);
    //         if (*currentPtr_A == '\n'){
    //             fprintf(out, "> ");
    //         }
    //     }

    //     if(count_A > 512) break;

    //     charCountPtr_A = currentPtr_A + 1;
    //     lineCharCount_A = (unsigned char) *charCountPtr_A + *(charCountPtr_A + 1) * 256;

    // }

    // // overflow case
    // if (ptr_d > hunk_deletions_buffer + 256){
    //     fprintf(out, "...\n");
    // }

}



/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */



static void copy_line(FILE *in, FILE *out){
    char c;
    // printf("copying ");
    do{
        c = getc(in);
        // printf("%c",c);
        if (!(global_options == 2 || global_options == 6)){
            fprintf(out, "%c", c);
        }
    }while (c != '\n');
    // printf("\n");
}

static int old_file_line_count = 1;
static int new_file_line_count = 1;
static int hunk_err = 0;
static int input_file_new_line_flag = 0;
int patch(FILE *in, FILE *out, FILE *diff) {

    // if any file is null pointer than return -1
    if (in == NULL || out == NULL || diff == NULL){
        // printf("INVALID FILE*\n");
        return -1;
    }

    HUNK hunk;
    hunk.serial = 0;
    int hunk_result = hunk_next(&hunk, diff);
    while (hunk_result == 0){

        // printf("hunk.serial: %d\n", hunk.serial);

        char c = hunk_getc(&hunk, diff);

        // printf("hunk_getc result: %c\n", c);

        int EOS_flag = 0;
        int parse_success_flag = 0;

        int run = 1;
        while (run){
            // printf("\n%c\t\t%d\n", c, c);

            // if we have EOS, ERR, then we successfully parse the hunk
            if (c - ERR == 0){
                // printf("ERROR\n");
                if (EOS_flag){
                    parse_success_flag = 1;
                }
                break;
            }

            // skip EOS
            if (c - EOS == 0){
                EOS_flag = 1;
                c = hunk_getc(&hunk, diff);
                continue;
            }
            // add c to the buffer according to CAT
            // printf("CHAR: %c, CAT: %c, old_file_line_count %d, new_file_line_count: %d\n", c, CAT, old_file_line_count, new_file_line_count);
            switch(CAT){
            case 'a':
                // modify output file under append action type

                if (change_type_flag){
                    // in change type, do not consider "skip line" scenario
                    // we also do not need to copy current line
                    if (!(global_options == 2 || global_options == 6)){
                        fprintf(out, "%c", c);
                    }
                }
                else{
                    // in append type, we need to consider "skip line" scenario
                    // then copy the hunk.old_start line
                    while (old_file_line_count <= hunk.old_start){
                        copy_line(in, out);
                        new_file_line_count++;
                        old_file_line_count++;
                    }
                    // now old_file_line_count = hunk.old_start + 1

                    // copy the new line not in old file
                    if (!(global_options == 2 || global_options == 6)){
                        fprintf(out, "%c", c);
                    }

                }
                if (c == '\n'){
                    new_file_line_count++;
                }

                break;
            case 'd':

                // modify output file under deletion action type
                while (old_file_line_count < hunk.old_start){
                    copy_line(in, out);
                    new_file_line_count++;
                    old_file_line_count++;
                }
                char c2 = fgetc(in);
                if (c2 - EOF == 0 && !input_file_new_line_flag){
                    c2 = '\n';
                }
                if (c2 != c){
                    // printf("%c,%c DOES NOT MATCH\n", c, c2);
                    // printf("%d,%d DOES NOT MATCH\n", c, c2);
                    run = 0;
                }

                if (c2 == '\n'){
                    input_file_new_line_flag = 1;
                }
                else{
                    input_file_new_line_flag = 0;
                }
                if (c == '\n'){
                    old_file_line_count++;
                }


                break;
            }


            if (run){
                c = hunk_getc(&hunk, diff);
            }


        }

        // printf("parse_success_flag: %d\n", parse_success_flag);
        // printf("old_file_line_count: %d, hunk_old_end: %d\n", old_file_line_count, hunk.old_end);
        // printf("new_file_line_count: %d, hunk_new_end: %d\n", new_file_line_count, hunk.new_end);
        if (old_file_line_count != hunk.old_end + 1 ||
            new_file_line_count != hunk.new_end + 1 ||
            !parse_success_flag){
            hunk_err = 1;

            if (global_options < 4){
                hunk_show(&hunk, stderr);
            }
            break;
        }
        hunk_result = hunk_next(&hunk, diff);



    }
    if (hunk_result != EOF){
        return -1;
    }
    if (!hunk_err){
        char c_in = fgetc(in);
        while (c_in - EOF != 0){
            if (!(global_options == 2 || global_options == 6)){
                fprintf(out, "%c", c_in);
            }
            c_in = fgetc(in);
        }
    }


    return hunk_err ? -1 : 0;
}
