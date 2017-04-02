#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cmdshell.h"

static void memory_error(void){
	errno = ENOMEM;
	perror(0);
	exit(1);
}


static void *xmalloc(size_t size){
	void *p = malloc(size);
	if (!p) memory_error();
	return p;
}


static void *xrealloc(void *ptr, size_t size){
	void *p = realloc(ptr, size);
	if (!p) memory_error();
	return p;
}


/* Read a line from standard input and put it in a char[] */
static char *readline(char *input){
	size_t l = strlen(input);
	if ((l > 0) && (input[l-1] == '\n')) {
		l--;
		input[l] = 0;
	}else if(l <= 0){
		return NULL;
	}
	return input;		
}


/* Split the string in words, according to the simple shell grammar. */
static char **split_in_words(char *line){
	char *cur = line;
	char **tab = 0;
	size_t l = 0;
	char c;

	while ((c = *cur) != 0) {
		char *w = 0;
		char *start;
		switch (c) {
		case ' ':
		case '\t':
			/* Ignore any whitespace */
			cur++;
			break;
		case '<':
			w = "<";
			cur++;
			break;
		case '>':
			w = ">";
			cur++;
			break;
		case '|':
			w = "|";
			cur++;
			break;
		case '&':
			w = "&";
			cur++;
			break;
		default:
			/* Another word */
			start = cur;
			while (c) {
				c = *++cur;
				switch (c) {
				case 0:
				case ' ':
				case '\t':
				case '<':
				case '>':
				case '|':
				case '&':
					c = 0;
					break;
				default: ;
				}
			}
			w = xmalloc((cur - start + 1) * sizeof(char));
			strncpy(w, start, cur - start);
			w[cur - start] = 0;
		}
		if (w) {
			tab = xrealloc(tab, (l + 1) * sizeof(char *));
			tab[l++] = w;
		}
	}
	tab = xrealloc(tab, (l + 1) * sizeof(char *));
	tab[l++] = 0;
	return tab;
}


static void freeseq(char ***seq){
	int i, j;

	for (i=0; seq[i]!=0; i++) {
		char **cmd = seq[i];

		for (j=0; cmd[j]!=0; j++) free(cmd[j]);
		free(cmd);
	}
	free(seq);
}


/* Free the fields of the structure but not the structure itself */
static void freecmd(struct cmdline *s){
	if (s->in) free(s->in);
	if (s->out) free(s->out);
	if (s->backgrounded) free(s->backgrounded);
	if (s->seq) freeseq(s->seq);
}


struct cmdline *readcmd(char * input){
	static struct cmdline *static_cmdline = 0;
	struct cmdline *s = static_cmdline;
	char *line;
	char **words;
	int i;
	char *w;
	char **cmd;
	char ***seq;
	size_t cmd_len, seq_len;

	line = readline(input);
	
	if (line == NULL) {
		if (s) {
			freecmd(s);
			free(s);
		}
		return static_cmdline = 0;
	}


	cmd = xmalloc(sizeof(char *));
	cmd[0] = 0;
	cmd_len = 0;
	seq = xmalloc(sizeof(char **));
	seq[0] = 0;
	seq_len = 0;

	words = split_in_words(line);

	if (!s)
		static_cmdline = s = xmalloc(sizeof(struct cmdline));
	else
		freecmd(s);
	s->err = 0;
	s->in = 0;
	s->out = 0;
	s->backgrounded = 0;
	s->seq = 0;

	i = 0;
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '&':
			if(s->backgrounded){
			s->err = "error on &";
			goto error;
			}
			s->backgrounded = &w[0];
			break;
		case '<':
			/* Tricky : the word can only be "<" */
			if (s->in) {
				s->err = "only one input file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for input redirection";
				goto error;
			}
			s->in = words[i++];
			break;
		case '>':
			/* Tricky : the word can only be ">" */
			if (s->out) {
				s->err = "only one output file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for output redirection";
				goto error;
			}
			s->out = words[i++];
			break;
		case '|':
			/* Tricky : the word can only be "|" */
			if (cmd_len == 0) {
				s->err = "misplaced pipe";
				goto error;
			}

			seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
			seq[seq_len++] = cmd;
			seq[seq_len] = 0;

			cmd = xmalloc(sizeof(char *));
			cmd[0] = 0;
			cmd_len = 0;
			break;
		default:
			cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
			cmd[cmd_len++] = w;
			cmd[cmd_len] = 0;
		}
	}

	if (cmd_len != 0) {
		seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
		seq[seq_len++] = cmd;
		seq[seq_len] = 0;
	} else if (seq_len != 0) {
		s->err = "misplaced pipe";
		i--;
		goto error;
	} else
		free(cmd);
	free(words);
	s->seq = seq;
	return s;
error:
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
		case '>':
		case '|':
		case '&':
			break;
		default:
			free(w);
		}
	}
	free(words);
	freeseq(seq);
	for (i=0; cmd[i]!=0; i++) free(cmd[i]);
	free(cmd);
	if (s->in) {
		free(s->in);
		s->in = 0;
	}
	if (s->out) {
		free(s->out);
		s->out = 0;
	}
	if (s->backgrounded) {
		free(s->backgrounded);
		s->out = 0;
	}
	return s;
}

int exec_cmd(char * input){
	int spid, status;
	struct cmdline *l;
	char ***seq;
	char **cmd;
	int before[2];
	int after[2];
	int pipe_fd[2];

	bool begin = true;

	l = readcmd(input);

	seq = l->seq;

	// printf("%s\n", **(l->seq));

	if (! *seq) return 1;

	if(!strcasecmp(**seq, "exit")) {
		return 0;
	}

	if(!strcasecmp(**seq, "cd")) {
		char *param = (*seq)[1];
		char *curr_dir = getcwd(NULL, 0);
		char *path;
		if (!param ) path = getenv("HOME");
		else if (strncmp(param,"/",1)) {
			path = strcat(curr_dir, "/");
			path = strcat(path, param);
		} else path = param;
		if (chdir(path)) {
			perror ("cd failed");
			chdir(curr_dir);
		}
		char * temp = "pwd";
		memset(l, 0, sizeof(l));
		l = readcmd(temp);
		printf("%s\n", **(l->seq));
		memset(&seq, 0, sizeof(seq));
		// l = readcmd("pwd");
		seq = l -> seq;
		// return 0;
	}
	int command_count;

	//count the number of command
	for(command_count = 0; seq[command_count]; command_count++);

	pid_t *pid= alloca(command_count * sizeof(pid_t));

	int process_number = 0;

	while(*seq){
		cmd = *seq;
		seq++;
		pipe(pipe_fd);
		if(*seq){
			pipe(after);
		}

		pid[process_number] = fork();

		if(!pid[process_number]){	//means that if pid == 0
			close(pipe_fd[0]);
			dup2(pipe_fd[1], 1);
			close(pipe_fd[1]);
			if(!begin){
				dup2(before[0], 0);
				close(before[0]);
				close(before[1]);
			}
			if(*seq){
				dup2(after[1], 1);
				close(after[0]);
				close(after[1]);
			}
			status = execvp(cmd[0], cmd);
		}else{
			if(!begin){
				close(before[0]);
				close(before[1]);
			}
			// memcpy(before, after, 2*sizeof(int));
			before[0] = after[0];


			before[1] = after[1];
			begin = false;
		}

		process_number++;
	}

	for(int i = 0; i< command_count; i++){
		waitpid(pid[i], NULL, 0);
	}

	return pipe_fd[0];
}
