%{
#include <stdio.h>
#include <string.h>
#include "./parse.h"
static YY_BUFFER_STATE scanbufhandle;
static char *scanbuf = NULL;
%}

%%
[-+*/:(),;]	{return yytext[0];}

[0-9]+		{yylval.float_val = strtod(yytext, (char **)NULL); return INTNUM;}

[0-9]+"."[0-9]* |
"."[0-9]*	{yylval.float_val = strtod(yytext, (char **)NULL); return FLOATNUM;}

[ \t\r]+	;
%%

int lexer_init(const char *str)
{
    if(str == NULL) return -1;
    unsigned int slen = strlen(str);
    if(slen <= 0) 
        return -1;

    if(YY_CURRENT_BUFFER)
        yy_delete_buffer(YY_CURRENT_BUFFER);

    scanbuf = (char *)malloc(slen + 2); 
    if(scanbuf == NULL)
        return -1;
    memcpy(scanbuf, str, slen);
    scanbuf[slen] = scanbuf[slen + 1] = YY_END_OF_BUFFER_CHAR;
    scanbufhandle = yy_scan_buffer(scanbuf, slen + 2); 
    return 0;
}

void lexer_finish(void)
{
    yy_delete_buffer(scanbufhandle);
    if(scanbuf) 
    {
        free(scanbuf);
        scanbuf = NULL;
    }
}
