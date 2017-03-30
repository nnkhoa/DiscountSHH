#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
// #include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cmdshell.h"

char* processinput(char* input){
	unsigned int length = sizeof(input);
	if((length > 0) && (input[length - 1 ] == '\n')){
		length--;
		input[length] = 0;
	}
	return input;	
}

char** parsecmd(char* cmd){
	char * current = cmd;
	char ** parse_cmd = 0;
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
			parse_cmd = realloc(parse_cmd, (length + 1) * sizeof(char *));
			parse_cmd[length++] = w;
		}
	}
	parse_cmd = realloc(parse_cmd, (length + 1) * sizeof(char *));
	parse_cmd[length++] = 0;
	return parse_cmd;
}

//free the sequence of commands
void free_sequence(char *** sequence){
	for (int i = 0; sequence[i] != 0; i++){
		char **section = sequence[i];

		for (int j = 0; section[j] != 0; j++){
			free(section[j]);
		}
		free(section);
	}
	free(sequence);
}

//free that data structure cmd
void free_cmd(struct cmd *_cmd){
	if(_cmd->error) free(_cmd->error);
	if(_cmd->input) free(_cmd->input);
	if(_cmd->output) free(_cmd->output);
	if(_cmd->background) free(_cmd->background);
	if(_cmd->sequence) free_sequence(_cmd->sequence);
}

struct cmd * parseinput(char * input){
	static struct cmd *static_cmd = 0;
	struct cmd *_cmd = static_cmd;
	char *w;
	char **section, **cmd_string;
	char ***sequence;
	unsigned int cmd_length, seq_length, i;

	input = processinput(input);

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
	section = parsecmd(input);

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
			sequence[seq_length++] = cmd_string;
			sequence[seq_length] = 0;
		}else if(seq_length != 0){
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

int exec_cmd(char *input){
	int spid, status;
	struct cmd * cmdline;
	char *** sequence;
	char **cmd_string;
	int before[2], after[2];

	cmdline = parseinput(input);
	sequence = cmdline->sequence;

	if(! *sequence){
		return 1;
	}

	if(!strcasecmp(**sequence, "cd")){
		char *param = (*sequence)[1];
		char *curr_dir = getcwd(NULL, 0);
		char *path;

		if(!param){
			path = getenv("HOME");
		}else if(strncmp(param, "/", 1)){
			path = strcat(curr_dir, "/");
			path = strcat(path, param);
		}else path = param;

		if(chdir(path)){
			perror("cd failed");
			chdir(curr_dir);
		}
		return 0;
	}

	int cmd_count;

	for(cmd_count = 0; sequence[cmd_count]; cmd_count++);

	pid_t *pid = alloca(cmd_count * sizeof(pid_t));
	
	int process_number = 0;

	bool begin = true;

	while(*sequence){
		cmd_string = *sequence;
		sequence++;
		if(*sequence){
			pipe(after);
		}
		pid[process_number] = fork();

		if(!pid[process_number]){
			if(!begin){
				dup2(before[0], 0);
				close(before[0]);
				close(before[1]);
			}
			if(*sequence){
				dup2(after[1],1);
				close(after[0]);
				close(after[1]);
			}
			status = execvp(cmd_string[0], cmd_string);
		}else{
			if(!begin){
				close(before[0]);
				close(before[1]);
			}

			memcpy(before, after, 2*sizeof(int));
			begin = false;
		}

		process_number++;
	}
	printf(".\n");
	for(int i = 0; i < cmd_count; i++){
		waitpid(pid[i], NULL, 0);
		printf("..\n");
	}

	return 0;
}