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
    /*  开始获取每一个token，然后分析这些token，每一个token就是有空格、字表符和回车符分隔的字符串
   */
    for (;;) {
        /*  next_token函数相当于词法分析器  */
        switch (next_token(&state)) {
        case T_EOF:  /*  init.rc文件分析完毕  */
            state.parse_line(&state, 0, 0);
            goto parser_done;
        case T_NEWLINE:  /*  分析每一行的命令  */
            /*  下面的代码相当于语法分析器  */
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
        case T_TEXT:  /*  处理每一个token  */
            if (nargs < INIT_PARSER_MAXARGS) {
                args[nargs++] = state.text;
            }
            break;
        }
    }
 
parser_done:
    /*  最后处理由import导入的初始化文件  */
    list_for_each(node, &import_list) {
         struct import *import = node_to_item(node, struct import, list);
         int ret;
 
         INFO("importing '%s'", import->filename);
         /*  递归调用  */ 
         ret = init_parse_config_file(import->filename);
         if (ret)
             ERROR("could not import file '%s' from '%s'\n",
                   import->filename, fn);
    }
}
