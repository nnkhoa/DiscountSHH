#ifndef _CMDSHELL_H
#define _CMDSHELL_H

struct cmd{
	char *error;		//error not null = there's a error
	char *input;		//input not null = file or name for input redirection
	char *output;		//output not null = file or name for output redirection
	char *background;	//background not null = processes in background
	char ***sequence;	//see below
};

//sequence here is a sequence of command, output of the previous one the input of the next
//are linked by a pipe
//a command is an array of string (char **), last item is a null pointer
//a sequence of command is an array of command (char ***), last item is a null pointer

int exec_cmd(char * input);

#endif