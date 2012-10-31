%{
#include <unistd.h>
#include "../lib/log.h"
int yylex(void);
extern "C" void yyerror(const char *str)
{ 
    ERROR_LOG("%s.", str);
}

extern "C" int yywrap(void) 
{ 
    return 1;
}

double *expr_result = NULL;
%}

%union
{
    double  float_val; 
}

%type <float_val> expr_result expr number INTNUM FLOATNUM
%token FLOATNUM INTNUM
/* operators */
%left '+' '-'
%left '*' '/'
%right UMINUS
%left '(' ')'

%%
expr_result : expr {if(expr_result) *expr_result = $1;}
expr: expr '+' expr {$$ = $1 + $3;}
    | expr '-' expr {$$ = $1 - $3;}
    | expr '*' expr {$$ = $1 * $3;}
    | expr '/' expr {$$ = $1 / $3;}
    | '+' expr  %prec UMINUS {$$ = $2;}
    | '-' expr  %prec UMINUS {$$ = -1 * $2;}
    | '(' expr ')'  {$$ = $2;}
    | number        {$$ = $1;}
    ;

number: INTNUM	  {$$ = $1;}
    |   FLOATNUM  {$$ = $1;}
    ;
%%
void parser_init(double *result)
{
    expr_result = result;
}
