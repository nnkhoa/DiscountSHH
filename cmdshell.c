#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "cmdshell.h"

char** parsecmd(char* cmd){
	char * current = cmd;
	char ** parse_cmd;
	unsigned int length;
	char c;

	while((c = *current) != 0){
		char * w = 0;
		char * start;

		switch(c){
			case ' ':
			case '\t':
				current++;
				break;
			case '<':
				w = "<";
				current ++;
				break;
			case '>':
				w = ">";
				current++;
				break;
			case '|':
				w = "|";
				current++;
				break;
			case '&':
				w = "&";
				current++;
				break;
			default:
				start = current;
				while(c){
					c = *++current;
					switch(c){
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
				w = malloc((current - start + 1)*sizeof(char));
				strncpy(w, start, current - start);
				w[current - start] = 0;
		}
		if(w){
			tab = realloc(tab, (length + 1) * sizeof(char *));
			tab[length++] = w;
		}
	}
	tab = realloc(tab, (length + 1) * sizeof(char *));
	tab[length++] = 0;
	return tab;
}

//free the sequence of commands
void free_sequence(char *** sequence){
	for (int i = 0; sequence[i] != 0; i++){
		char section = sequence[i];

		for (int j = 0; section[j] != 0; j++){
			free(section[j]);
		}
		free(section);
	}
	free(sequence);
}

//free that data structure cmd
void free_cmd(struct cmd *_cmd){
	if(_cmd->error) free(error);
	if(_cmd->input) free(_cmd->input);
	if(_cmd->output) free(_cmd->output);
	if(_cmd->background) free(_cmd->background);
	if(_cmd->sequence) free_sequence(_cmd->sequence);
}

struct cmd *parseinput(char * input){
	static struct cmd *static_cmd = 0;
	struct cmd *_cmd = *static_cmd;
	char *w;
	char **section, **cmd_string;
	char ***sequence;
	unsigned int cmd_length, seq_length, i;

	if(input == NULL){
		if(cmd_string){
			free_cmd(_cmd);
			free(cmd_string);
		}
		return static_cmd = 0;
	}

	cmd_string = malloc(sizeof(char *));
	cmd_string[0] = 0;
	cmd_length = 0;

	sequence = malloc(sizeof(char **));
	sequence[0] = 0;
	seq_length = 0;

	//should check if i need to remove the '/n' symbol in input or not
	section = parseinput(input);

	if(!_cmd){
		static_cmd = _cmd = malloc(sizeof(struct cmd));
	}else{
		free_cmd(_cmd);
	}

	memset(_cmd, 0, sizeof(_cmd));

	i = 0;

	while((w = section[i++]) != 0){
		switch(w[0]){
			case '&':
				if(_cmd->background){
					_cmd->error = "error on &";
					goto error;
				}
				_cmd->background = &w[0];
				break;
			case '<':
				if(_cmd->input){
					_cmd->error = "only 1 input file is allowed";
					goto error;
				}
				if(section[i] == 0){
					_cmd->error = "file name missing for input redirection";
					goto error;
				}
				_cmd->input = section[i++];
				break;
			case '>':
				if(_cmd->output){
					_cmd->error = "only 1 output file is allowed";
					goto error;
				}
				if(section[i] == 0){
					_cmd->error = "file name missing for output redirection";
					goto error;
				}
				_cmd->output = section[i++];
				break;
			case '|':
				if(cmd_length == 0){
					_cmd->error = "misplace pipe";
					goto error;
				}

				sequence = realloc(sequence, (seq_length + 2)*sizeof(char **));
				sequence[seq_length++] = cmd_string;
				sequence[seq_length] = 0;

				cmd_string = malloc(sizeof(char *));
				cmd_string[0] = 0;
				cmd_length = 0;
				break;
			default:
				cmd_string = realloc(cmd_string, (cmd_length + 2)*sizeof(char *));
				cmd_string[cmd_length++] = w;
				cmd_string[cmd_length] = 0;
		}

		if(cmd_length != 0){
			sequence = realloc(sequence, (seq_length + 2)*sizeof(char **));
			sequence[seq_length++] = cmd_length;
			sequence[seq_length] = 0;
		}else if(seq_length ! = 0){
			_cmd -> error = "misplace pipe";
			i--;
			goto error;
		}else{
			free(cmd_string);
		}
		free(section);
		_cmd->sequence = sequence;
		return _cmd;
	}
error:
	while((w = section[i++]) != 0){
		switch(w[0]){
			case '&':
			case '<':
			case '>':
			case '|':
				break;
			default:
				free(w);
		}
	}
	free(section);
	free_sequence(sequence);
	for(i = 0; cmd_string[i] != 0; i++){
		free(cmd_string[i]);
	}
	free(cmd_string);

	if(_cmd->input){
		free(_cmd->input);
		_cmd->input = 0;
	}
	if(_cmd->output){
		free(_cmd->output);
		_cmd->output = 0;
	}
	if(_cmd->background){
		free(_cmd->background);
		_cmd->background = 0;
	}
	return _cmd;
}