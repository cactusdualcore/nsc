#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "../include/nested_text.h"

void printNestedText(struct NestedText *const nt) {
    const size_t item_size = nt->item_end - nt->item_start;

    char *const buffer = malloc(item_size + 1);
    memcpy(buffer, nt->item_start, item_size);
    buffer[item_size] = '\0';

    if (nt->type != NT_LINE_T_UNKNOWN) {
        printf("%02d ", nt->depth);
    }

    switch (nt->type) {
        case NT_LINE_T_COMMENT:
            printf("comment    = ");
            break;
        case NT_LINE_T_STRING_ITEM:
            printf("string     = ");
            break;
        case NT_LINE_T_LIST_ITEM:
            printf("list item  = ");
            break;
        case NT_LINE_T_DICT_KEY:
            printf("dict key   = ");
            break;
        case NT_LINE_T_DICT_VALUE:
            printf("dict value = ");
            break;
        case NT_LINE_T_UNKNOWN:
            fprintf(stderr, "unknown line type.\n");
            exit(EX_DATAERR);
    }

    printf("%s\n", buffer);
    free(buffer);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "incorrect number of arguments: %d\n", argc - 1);
        exit(EX_USAGE);
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        perror(NULL);
        exit(EX_NOINPUT);
    }

    char *const buffer = malloc(8192);

    errno = 0;
    const unsigned long bytes_read = fread(buffer, 1, 8192, file);
    if (errno != 0) {
        perror(NULL);
        exit(EX_IOERR);
    }

    struct NestedText nt;

    NestedText_init(&nt, buffer, bytes_read);

    enum NestedTextResult result;
    while ((result = NestedText_parse(&nt)) != NT_DONE) {
        switch (result) {
            case NT_OK:
                printNestedText(&nt);
                break;
            case NT_NEEDS_VALUE:
                if (nt.type == NT_LINE_T_DICT_KEY) {
                    printNestedText(&nt);
                }
                NestedText_value(&nt);
                printNestedText(&nt);
                break;
            case NT_UNEXPECTED:
                fprintf(stderr, "unexpected character: '%c'\n", *nt.cursor);
                exit(EX_DATAERR);
            case NT_DONE:
                __builtin_unreachable();
        }
    }

    return EX_OK;
}
