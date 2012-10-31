/////////////////////////////////////

#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include "log.h"
#include "oa_popen.h"

#ifdef	OPEN_MAX
static int openmax = OPEN_MAX;
#else
static int openmax = 0;
#endif
#define	OPEN_MAX_GUESS	256		

pid_t *g_childpid = NULL;          //保存子进程id的数组
int   *g_child_stderr_array = NULL;//保存子进程错误终端fd的数组

static int g_maxfd;	
static volatile int g_childtermd = 0;

int get_open_max(void)
{
    if(openmax == 0) {						
        errno = 0;
        if((openmax = sysconf(_SC_OPEN_MAX)) < 0) {
            if(errno == 0) {
                openmax = OPEN_MAX_GUESS;	
            }
        }
    }

    return openmax;
}

int oa_popen_init()
{
    g_maxfd = get_open_max();	
    if(g_childpid == NULL) {		
        if((g_childpid = (pid_t*)calloc((size_t)g_maxfd, sizeof(pid_t))) == NULL) {
            ERROR_LOG("ERROR: calloc for g_childpid failed.");
            return -1;
        }
    }

    if(g_child_stderr_array == NULL) {	
        if((g_child_stderr_array = (int*)calloc((size_t)g_maxfd, sizeof(int))) == NULL) {
            ERROR_LOG("ERROR: calloc for g_child_stderr_array failed.");
            if(g_childpid) {
                free(g_childpid);
                g_childpid = NULL;
            }
            return -1;
        }
    }

    return 0;
}

void oa_popen_uninit()
{
    if(g_childpid != NULL && g_child_stderr_array != NULL) {
        for(int i = 0; i < g_maxfd; i++) {
            if(g_childpid[i] != 0) {
                kill(g_childpid[i], SIGKILL);
            }
            if(g_child_stderr_array[i] > 0) {
                close(g_child_stderr_array[i]);
            }
        }
        free(g_childpid);
        free(g_child_stderr_array);
    }
    return;
}

FILE *oa_popen(const char *cmdstring)
{
    char  *env[2] = {NULL};
    char   cmd[4096 + 1] = {0};
    char  *argv[1024] = {NULL};
    char  *str = NULL, *tmp = NULL;
    int    argc = 0;
    int    i = 0, pfd[2] = {-1}, pfderr[2] = {-1};
    pid_t  pid;

#ifdef 	RLIMIT_CORE
    //不产生core文件
    struct rlimit limit;
    getrlimit(RLIMIT_CORE, &limit);
    limit.rlim_cur = 0;
    setrlimit(RLIMIT_CORE, &limit);
#endif

    env[0] = strdup("LC_ALL=C");
    env[1] = '\0';

    if(cmdstring == NULL) {      
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        ERROR_LOG("Cmdstring is null.");
        return NULL;
    }

    strncpy(cmd, cmdstring, sizeof(cmd) - 1);

    if(strstr(cmdstring, "\"") || strstr(cmdstring, " ' ") || strstr(cmdstring, "'''")) {
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        ERROR_LOG("Invalid argument.");
        return NULL;
    }

    argc = strlen(cmdstring) + 1;
    char  *start = cmd;

    while(start) {
        str = start;
        str += strspn(str, " \t\r\n");

        if(i >= argc - 2) {
            ERROR_LOG("CRITICAL - too few argument.");
#ifdef 	RLIMIT_CORE
            limit.rlim_cur = RLIM_INFINITY;
            setrlimit(RLIMIT_CORE, &limit);
#endif
            return NULL;
        }

        if(strstr(str, "'") == str) {
            str++;
            if(!strstr(str, "'")) {
#ifdef 	RLIMIT_CORE
                limit.rlim_cur = RLIM_INFINITY;
                setrlimit(RLIMIT_CORE, &limit);
#endif
                ERROR_LOG("Invalid argument.");
                return NULL;
            }
            start = strstr(str, "'") + 1;
            str[strcspn(str, "'")] = 0;
        } else if(strcspn(str, "'") < strcspn(str, " \t\r\n")) {
            tmp = str + strcspn(str, "'") + 1; 
            if(!strstr(tmp, "'")) {
#ifdef 	RLIMIT_CORE
                limit.rlim_cur = RLIM_INFINITY;
                setrlimit(RLIMIT_CORE, &limit);
#endif
                ERROR_LOG("Invalid argument.");
                return NULL;
            }
            tmp += strcspn(tmp,"'") + 1;
            *tmp = 0;
            start = tmp + 1;
        } else {
            if(strpbrk(str, " \t\r\n")) {
                start = 1 + strpbrk(str, " \t\r\n");
                str[strcspn(str, " \t\r\n")] = 0;
            } else {
                start = NULL;
            }
        }

        if(start && strlen(start) == strspn(start, " \t\r\n")) {
            start = NULL;
        }

        argv[i++] = str;
    }
    argv[i] = NULL;

    if(pipe(pfd) < 0) {
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        ERROR_LOG("Create stdout pipe failed.");
        return NULL;
    }

    if(pipe(pfderr) < 0) {
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        close(pfd[0]);
        close(pfd[1]);
        ERROR_LOG("Create stderr pipe failed.");
        return NULL;
    }

    if(signal(SIGCHLD, popen_sigchld_handler) == SIG_ERR) {
        ERROR_LOG("Cannot catch SIGCHLD.");
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        close(pfd[0]);
        close(pfd[1]);
        close(pfderr[0]);
        close(pfderr[1]);
        return NULL;
    }

    if((pid = fork()) < 0) {
        ERROR_LOG("Fork failed.");
#ifdef 	RLIMIT_CORE
        limit.rlim_cur = RLIM_INFINITY;
        setrlimit(RLIMIT_CORE, &limit);
#endif
        close(pfd[0]);
        close(pfd[1]);
        close(pfderr[0]);
        close(pfderr[1]);
        return NULL;
    } else if(pid == 0){//子进程
        close(pfd[0]);
        if(pfd[1] != STDOUT_FILENO) {
            dup2(pfd[1], STDOUT_FILENO); 
            close(pfd[1]);
        }
        close(pfderr[0]);
        if(pfderr[1] != STDERR_FILENO) {
            dup2(pfderr[1], STDERR_FILENO);
            close(pfderr[1]);
        }

        for(i = 0; i < g_maxfd; i++) {
            if(g_childpid[i] > 0) {
                close(i);
            }
        }
        execve(argv[0], argv, env);
        _exit(127);
    }

    close(pfd[1]);								
    close(pfderr[1]);

    FILE *child_stdout = NULL;
    if((child_stdout = fdopen(pfd[0], "r")) == NULL) {
        return NULL;
    }

    g_childpid[fileno(child_stdout)] = pid;	
    g_child_stderr_array[fileno(child_stdout)] = pfderr[0];	
    return child_stdout;
}

int oa_pclose(FILE * fp)
{
    int fd, status;
    pid_t pid;

    if(g_childpid == NULL) {
        return -1;	
    }

    fd = fileno(fp);
    if((pid = g_childpid[fd]) == 0) {
        return -1;		
    }

    g_childpid[fd] = 0;
    if(fclose(fp) == EOF) {
        return -1;
    }

    while(waitpid(pid, &status, 0) < 0) {
        if(errno != EINTR) {
            return -1;							
        }
    }

    if(WIFEXITED(status)) {
        return(WEXITSTATUS(status));
    }

    return -1;
}

void popen_sigchld_handler(int signo)
{
    if(signo == SIGCHLD) {
        g_childtermd = 1;
    }
}

