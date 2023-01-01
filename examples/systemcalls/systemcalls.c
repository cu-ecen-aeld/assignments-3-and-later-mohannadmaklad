#include "systemcalls.h"
#include "stdlib.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>


/*
 * @Description Logs the command arguments to be executed for debugging purposes
 *
 * */
void log_commands(int count, char* cmds[]){
	for(int i=0; i<count; i++){
		syslog(LOG_DEBUG, "Command %d is %s", i, cmds[i]);
	}
}


/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    return system(cmd) == 0;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    /* Prepare syslog */
    openlog("SystemCalls", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Starting system log");

    pid_t pid;
    switch(pid=fork()){
	 case -1:
		 syslog(LOG_INFO,"Child process couldn't be created!"); 
		 exit(1);
	 case 0: {
             /* Child Code */
	     syslog(LOG_INFO,"Child process is created! Executing the command in path %s", command[0]);
             log_commands(count, command);
	     /* Execute the command in the child process */
	     if(execv(command[0], command) == -1) {
	       syslog(LOG_INFO, "Couldn't execute the command with path: %s", command[0]);
	       exit(1);
	     }
	}
    }
     
    /* Parent Code */
    int wstatus=0;
    if(wait(&wstatus) == -1) return 0;;
    
    syslog(LOG_DEBUG, "WIFEXITED: %d, and WEXITSTATUS: %d",WIFEXITED(wstatus), WEXITSTATUS(wstatus));
    syslog(LOG_DEBUG, "Return value %d",(WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS));
    closelog();
    va_end(args);

    return (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    /* Open the file to redirect to */
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    
    openlog("SystemCalls", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Starting system log");
    
    pid_t pid;
    switch(pid=fork()){
	 case -1:
		 syslog(LOG_INFO,"Child process couldn't be created!"); 
		 exit(1);
	 case 0: {
	     /* Child process code */
	     syslog(LOG_INFO,"Child process is created! Executing the command in path %s", command[0]);
             log_commands(count, command);
             /* Redirect the output to the passed file */
	     dup2(fd,1);
             /* Execute the command in the child process */
	     if(execv(command[0], command) == -1) {
	       syslog(LOG_INFO, "Couldn't execute the command with path: %s", command[0]);
	       exit(1);
	     }
	}
    }

    int wstatus=0;
    if(wait(&wstatus) == -1) return 0;;
    
    syslog(LOG_DEBUG, "WIFEXITED: %d, and WEXITSTATUS: %d",WIFEXITED(wstatus), WEXITSTATUS(wstatus));
    syslog(LOG_DEBUG, "Return value %d",(WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS));
    closelog();
    va_end(args);
    
    return (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_SUCCESS); 
}
