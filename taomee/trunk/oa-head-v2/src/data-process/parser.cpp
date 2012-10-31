///author :mason

#include <unistd.h>
#include <pthread.h>

#include "./parse.h"
#include "./parser.h"
extern int lexer_init(const char *str);
extern void lexer_finish(void);
extern void parser_init(double *);
extern int yyparse(void);

pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

int do_parse(const char *str, double *result)
{
    if(str == NULL || result == NULL) {
        return -1;
    }

    pthread_mutex_lock(&mutex_lock);
    if(lexer_init(str) != 0) {
        pthread_mutex_unlock(&mutex_lock);
        return -1;
    }

    parser_init(result);
    int yyresult = yyparse();
    lexer_finish();
    pthread_mutex_unlock(&mutex_lock);

    if(yyresult) {
        return -1;
    }

    return 0;
}

