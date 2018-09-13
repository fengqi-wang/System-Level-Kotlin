static void parse_config(const char *fn, char *s)
{
    struct parse_state state;
    struct listnode import_list;
    struct listnode *node;
    char *args[INIT_PARSER_MAXARGS];
    int nargs;
 
    nargs = 0;
    state.filename = fn;
    state.line = 0;
    state.ptr = s;
    state.nexttoken = 0;
    state.parse_line = parse_line_no_op;
 
    list_init(&import_list);
    state.priv = &import_list;
    
    for (;;) {
        switch (next_token(&state)) {
        case T_EOF:  
            state.parse_line(&state, 0, 0);
            goto parser_done;
        case T_NEWLINE: 
            state.line++;
            if (nargs) {
                int kw = lookup_keyword(args[0]);
                if (kw_is(kw, SECTION)) {
                    state.parse_line(&state, 0, 0);
                    parse_new_section(&state, kw, nargs, args);
                } else {
                    state.parse_line(&state, nargs, args);
                }
                nargs = 0;
            }
            break;
        case T_TEXT:
            if (nargs < INIT_PARSER_MAXARGS) {
                args[nargs++] = state.text;
            }
            break;
        }
    }
 
parser_done:
    list_for_each(node, &import_list) {
         struct import *import = node_to_item(node, struct import, list);
         int ret;
 
         INFO("importing '%s'", import->filename);
         ret = init_parse_config_file(import->filename);
         if (ret)
             ERROR("could not import file '%s' from '%s'\n",
                   import->filename, fn);
    }
}

int next_token(struct parse_state *state)
{
    char *x = state->ptr;
    char *s;
 
    if (state->nexttoken) {
        int t = state->nexttoken;
        state->nexttoken = 0;
        return t;
    }
    /*  在这里开始一个字符一个字符地分析  */
    for (;;) {
        switch (*x) {
        case 0:
            state->ptr = x;
            return T_EOF;
        case '\n':
            x++;
            state->ptr = x;
            return T_NEWLINE;
        case ' ':
        case '\t':
        case '\r':
            x++;
            continue;
        case '#':
            while (*x && (*x != '\n')) x++;
            if (*x == '\n') {
                state->ptr = x+1;
                return T_NEWLINE;
            } else {
                state->ptr = x;
                return T_EOF;
            }
        default:
            goto text;
        }
    }
 
textdone:
    state->ptr = x;
    *s = 0;
    return T_TEXT;
text:
    state->text = s = x;
textresume:
    for (;;) {
        switch (*x) {
        case 0:
            goto textdone;
        case ' ':
        case '\t':
        case '\r':
            x++;
            goto textdone;
        case '\n':
            state->nexttoken = T_NEWLINE;
            x++;
            goto textdone;
        case '"':
            x++;
            for (;;) {
                switch (*x) {
                case 0:
                        /* unterminated quoted thing */
                    state->ptr = x;
                    return T_EOF;
                case '"':
                    x++;
                    goto textresume;
                default:
                    *s++ = *x++;
                }
            }
            break;
        case '\\':
            x++;
            switch (*x) {
            case 0:
                goto textdone;
            case 'n':
                *s++ = '\n';
                break;
            case 'r':
                *s++ = '\r';
                break;
            case 't':
                *s++ = '\t';
                break;
            case '\\':
                *s++ = '\\';
                break;
            case '\r':
                    /* \ <cr> <lf> -> line continuation */
                if (x[1] != '\n') {
                    x++;
                    continue;
                }
            case '\n':
                    /* \ <lf> -> line continuation */
                state->line++;
                x++;
                    /* eat any extra whitespace */
                while((*x == ' ') || (*x == '\t')) x++;
                continue;
            default:
                    /* unknown escape -- just copy */
                *s++ = *x++;
            }
            continue;
        default:
            *s++ = *x++;
        }
    }
    return T_EOF;
}

int lookup_keyword(const char *s)
{
    switch (*s++) {
    case 'c':
    if (!strcmp(s, "opy")) return K_copy;
        if (!strcmp(s, "apability")) return K_capability;
        if (!strcmp(s, "hdir")) return K_chdir;
        if (!strcmp(s, "hroot")) return K_chroot;
        if (!strcmp(s, "lass")) return K_class;
        if (!strcmp(s, "lass_start")) return K_class_start;
        if (!strcmp(s, "lass_stop")) return K_class_stop;
        if (!strcmp(s, "lass_reset")) return K_class_reset;
        if (!strcmp(s, "onsole")) return K_console;
        if (!strcmp(s, "hown")) return K_chown;
        if (!strcmp(s, "hmod")) return K_chmod;
        if (!strcmp(s, "ritical")) return K_critical;
        break;
    case 'd':
        if (!strcmp(s, "isabled")) return K_disabled;
        if (!strcmp(s, "omainname")) return K_domainname;
        break;
     … …
    case 'o':
        if (!strcmp(s, "n")) return K_on;
        if (!strcmp(s, "neshot")) return K_oneshot;
        if (!strcmp(s, "nrestart")) return K_onrestart;
        break;
    case 'r':
        if (!strcmp(s, "estart")) return K_restart;
        if (!strcmp(s, "estorecon")) return K_restorecon;
        if (!strcmp(s, "mdir")) return K_rmdir;
        if (!strcmp(s, "m")) return K_rm;
        break;
    case 's':
        if (!strcmp(s, "eclabel")) return K_seclabel;
        if (!strcmp(s, "ervice")) return K_service;
        if (!strcmp(s, "etcon")) return K_setcon;
        if (!strcmp(s, "etenforce")) return K_setenforce;
        if (!strcmp(s, "etenv")) return K_setenv;
        if (!strcmp(s, "etkey")) return K_setkey;
        if (!strcmp(s, "etprop")) return K_setprop;
        if (!strcmp(s, "etrlimit")) return K_setrlimit;
        if (!strcmp(s, "etsebool")) return K_setsebool;
        if (!strcmp(s, "ocket")) return K_socket;
        if (!strcmp(s, "tart")) return K_start;
        if (!strcmp(s, "top")) return K_stop;
        if (!strcmp(s, "ymlink")) return K_symlink;
        if (!strcmp(s, "ysclktz")) return K_sysclktz;
        break;
    case 't':
        if (!strcmp(s, "rigger")) return K_trigger;
        break;
    case 'u':
        if (!strcmp(s, "ser")) return K_user;
        break;
    case 'w':
        if (!strcmp(s, "rite")) return K_write;
        if (!strcmp(s, "ait")) return K_wait;
        break;
    }
    return K_UNKNOWN;
}

#include "keywords.h"
#define KEYWORD(symbol, flags, nargs, func) \
    [ K_##symbol ] = { #symbol, func, nargs + 1, flags, },
struct {
    const char *name;
    int (*func)(int nargs, char **args);
    unsigned char nargs;
    unsigned char flags;
} keyword_info[KEYWORD_COUNT] = {
    [ K_UNKNOWN ] = { "unknown", 0, 0, 0 },
#include "keywords.h"
};

void parse_new_section(struct parse_state *state, int kw,
                       int nargs, char **args)
{
    printf("[ %s %s ]\n", args[0],
           nargs > 1 ? args[1] : "");
    switch(kw) {
    case K_service:  //  处理service
        state->context = parse_service(state, nargs, args);
        if (state->context) {
            state->parse_line = parse_line_service;
            return;
        }
        break;
    case K_on:  //  处理action
        state->context = parse_action(state, nargs, args);
        if (state->context) {
            state->parse_line = parse_line_action;
            return;
        }
        break;
    case K_import:   //  单独处理import导入的初始化文件。
        parse_import(state, nargs, args);
        break;
    }
    state->parse_line = parse_line_no_op;
}


static void parse_line_service(struct parse_state *state, int nargs, char **args)
{
    struct service *svc = state->context;
    struct command *cmd;
    int i, kw, kw_nargs;
 
    if (nargs == 0) {
        return;
    }
 
    svc->ioprio_class = IoSchedClass_NONE;
 
    kw = lookup_keyword(args[0]);
    //  下面处理每一个option
    switch (kw) {
    case K_capability:
        break;
    … …
    case K_group:
        if (nargs < 2) {
            parse_error(state, "group option requires a group id\n");
        } else if (nargs > NR_SVC_SUPP_GIDS + 2) {
            parse_error(state, "group option accepts at most %d supp. groups\n",
                        NR_SVC_SUPP_GIDS);
        } else {
            int n;
            svc->gid = decode_uid(args[1]);
            for (n = 2; n < nargs; n++) {
                svc->supp_gids[n-2] = decode_uid(args[n]);
            }
            svc->nr_supp_gids = n - 2;
        }
        break;
    case K_keycodes:
        if (nargs < 2) {
            parse_error(state, "keycodes option requires atleast one keycode\n");
        } else {
            svc->keycodes = malloc((nargs - 1) * sizeof(svc->keycodes[0]));
            if (!svc->keycodes) {
                parse_error(state, "could not allocate keycodes\n");
            } else {
                svc->nkeycodes = nargs - 1;
                for (i = 1; i < nargs; i++) {
                    svc->keycodes[i - 1] = atoi(args[i]);
                }
            }
        }
        break;
        … …
     }
    ……
}

