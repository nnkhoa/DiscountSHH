#ifndef _CMDSHELL_H
#define _CMDSHELL_H

/* Read a command line from input stream. Return null when input closed.
Display an error and call exit() in case of memory exhaustion. */
struct cmdline *readcmd(char * input);


/* Structure returned by readcmd() */
struct cmdline {
	char *err;	/* If not null, it is an error message that should be
			   displayed. The other fields are null. */
	char *in;	/* If not null : name of file for input redirection. */
	char *out;	/* If not null : name of file for output redirection. */
	char *backgrounded; /* If not null : processus is backgrounded */       //added
	char ***seq;	/* See comment below */
};

int exec_cmd(char * input);


#endif