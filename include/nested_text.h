#ifndef NESTED_TEXT_H
#define NESTED_TEXT_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum NestedTypeLineType {
    NT_LINE_T_COMMENT = 0,
    NT_LINE_T_STRING_ITEM = 1,
    NT_LINE_T_LIST_ITEM = 2,
    NT_LINE_T_DICT_KEY = 3,
    NT_LINE_T_DICT_VALUE = 4,
    NT_LINE_T_UNKNOWN = -1,
};

struct NestedText {
    const char *item_start;
    const char *item_end;
    const char *cursor;
    const char *end;
    enum NestedTypeLineType type;
    int previous_indent_level;
    int depth;
};

void NestedText_init(struct NestedText *const nt, const char *const s, const size_t len) {
    nt->item_start = s;
    nt->item_end = s;
    nt->cursor = s;
    nt->end = s + len;
    nt->type = NT_LINE_T_UNKNOWN;
    nt->previous_indent_level = 0;
    nt->depth = 0;
}

enum NestedTextResult {
    NT_OK = 0,
    NT_DONE = 1,
    NT_NEEDS_VALUE = 2,
    NT_UNEXPECTED = -1,
};

enum NestedTextResult __NT_line(struct NestedText *const nt);

enum NestedTextResult __NT_blank_line(struct NestedText *const nt);

enum NestedTextResult __NT_list_item(struct NestedText *const nt);

enum NestedTextResult __NT_string_item(struct NestedText *const nt);

enum NestedTextResult __NT_assume_dict_key(struct NestedText *const nt);

enum NestedTextResult NestedText_parse(struct NestedText *const nt) {
    return __NT_line(nt);
}

enum NestedTextResult NestedText_value(struct NestedText *const nt);

#define __NT_SPACE ' '
#define __NT_LF '\n'
#define __NT_CR '\r'

enum NestedTextResult __NT_line(struct NestedText *const nt) {
line_skipped:
    if (nt->cursor == nt->end) return NT_DONE;

    int indent_level = 0;
    while (*nt->cursor == __NT_SPACE) {
        nt->cursor++;
        indent_level++;
    }

    if (nt->previous_indent_level < indent_level) {
        nt->depth++;
    } else if (nt->previous_indent_level > indent_level) {
        nt->depth--;
    }

    nt->item_start = nt->cursor;

    enum NestedTextResult result;
    switch (*nt->cursor++) {
        case '\n':
        case '\r':
            __NT_blank_line(nt);
            goto line_skipped;
        case '#':
            while (*nt->cursor != __NT_LF && *nt->cursor != __NT_CR)
                nt->cursor++;
            goto line_skipped;
        case '-':
            result = __NT_list_item(nt);
            nt->type = NT_LINE_T_LIST_ITEM;
            break;
        case '>':
            result = __NT_string_item(nt);
            nt->type = NT_LINE_T_STRING_ITEM;
            break;
        case '[':
            // TODO(cactusdualcore) implement inline list
            break;
        case '{':
            // TODO(cactusdualcore) implement inline dict
            break;
        default:
            result = __NT_assume_dict_key(nt);
            switch (result) {
                case NT_OK:
                case NT_NEEDS_VALUE:
                    nt->type = NT_LINE_T_DICT_KEY;
                    break;
                case NT_UNEXPECTED:
                    nt->type = NT_LINE_T_UNKNOWN;
                    break;
                case NT_DONE: // unreachable
                    break;
            }
            break;
    }

    nt->previous_indent_level = indent_level;
    return result;
}

enum NestedTextResult __NT_blank_line(struct NestedText *const nt) {
    if (*nt->cursor == '\n') nt->cursor++;
    return NT_OK;
}

enum NestedTextResult __NT_list_item(struct NestedText *const nt) {
    switch (*nt->cursor++) {
        case __NT_SPACE:
            return NT_NEEDS_VALUE;
        case __NT_LF:
        case __NT_CR:
            return NT_OK;
        default:
            nt->cursor--;
            return NT_UNEXPECTED;
    }
}

enum NestedTextResult __NT_string_item(struct NestedText *const nt) {
    switch (*nt->cursor++) {
        case __NT_SPACE:
            return NestedText_value(nt);
            break;
        case __NT_LF:
        case __NT_CR:
            break;
        default:
            nt->cursor--;
            return NT_UNEXPECTED;
    }
    return NT_OK;
}

enum NestedTextResult __NT_assume_dict_key(struct NestedText *const nt) {
    while (*nt->cursor != __NT_LF && *nt->cursor != __NT_CR && *nt->cursor != ':')
        nt->cursor++;
    if (*nt->cursor == ':') {
        nt->item_end = nt->cursor;
        switch (*++nt->cursor) {
            case __NT_SPACE:
                nt->cursor++;
                return NT_NEEDS_VALUE;
            case __NT_LF:
            case __NT_CR:
                nt->cursor++;
                return NT_OK;
            default:
                return NT_UNEXPECTED;
        }
    } else {
        return NT_UNEXPECTED;
    }
}

enum NestedTextResult NestedText_value(struct NestedText *const nt) {
    nt->item_start = nt->cursor;
    while (*nt->cursor != __NT_LF && *nt->cursor != __NT_CR) nt->cursor++;
    nt->item_end = nt->cursor;
    nt->cursor++;
    nt->type = NT_LINE_T_DICT_VALUE;
    return NT_OK;
}

#undef __NT_SPACE
#undef __NT_LF
#undef __NT_CR

#endif