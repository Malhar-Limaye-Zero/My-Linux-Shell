#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()


char parseInput(char* input_str, char** command_arr) //parsed commands stored into command_arr
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).
	char ret_delimit; //return value
	int in_len = strlen(input_str);
	char* str_nonewline = (char *)malloc(in_len-1);
	for(int j = 0; j < in_len-1; j++)
		str_nonewline[j] = input_str[j];
	strcat(str_nonewline," ");
	//check which delimiter present and seperate accordingly
	if(strstr(str_nonewline,"&&")!=NULL)
	{
		ret_delimit='&';//returnvalue
		//put commands into the array
		int i = 0;
		char* sep_str;
		while((sep_str = strsep(&str_nonewline,"&&")) != NULL )
		{
			int count = 0;
			// store the isolated commands in command_list
			strcpy(command_arr[i],sep_str);
			i++;
		}
		command_arr[i]=NULL;
	}
	else if(strstr(str_nonewline,"##")!=NULL)
	{
		ret_delimit='#';//returnvalue
		//put commands into the array
		int i = 0;
		char* sep_str;
		while((sep_str = strsep(&str_nonewline,"##")) != NULL )
		{
			int count = 0;
			// store the isolated commands in command_list
			strcpy(command_arr[i],sep_str);
			i++;
		}
		command_arr[i]=NULL;
	}
	else if(strstr(str_nonewline,">")!=NULL)
	{
		ret_delimit='>';//returnvalue
		//put commands into the array
		int i = 0;
		char* sep_str;
		while((sep_str = strsep(&str_nonewline,">")) != NULL )
		{
			int count = 0;
			// store the isolated commands in command_list
			strcpy(command_arr[i],sep_str);
			i++;
		}
		command_arr[i]=NULL;
	}
	else
	{
		//no delimiters implies only space present, so store as single command for now, then seperate into arguments list
		char* token = str_nonewline;
		command_arr[0]=token;
		command_arr[1]=NULL;
		ret_delimit=' ';
	}
	return ret_delimit;
}
//Auxillary function to change command string into argument list
char** get_commandArgs(char* command)
{
	//remove new line character from the string
	command = strsep(&command, "\n");		

	// remove whitespaces before a command if there are present
	while(*command == ' ')					
		command++;
	
	// create duplicate command string
	char* dup_command = strdup(command);

	// get the command in a string, to check for "cd"
	char* first_sep = strsep(&command, " ");

	// change directory is "cd" is encountered
	if (strcmp(first_sep,"cd")==0)
	{
		chdir(strsep(&command, "\n "));
	}
	else
	{
		// allocate memory to the array of strings, to store the command its arguments
		char** command_args;
		command_args = (char**)malloc(sizeof(char*)*10);
		for(int i=0; i<10; i++)
			command_args[i] = (char*)malloc(50*sizeof(char));

		char* sep_str;
		int i = 0;

		//if there's a string with spaces, else directly assign the string to command_args[i]
		if (strstr(dup_command, " ")!=NULL)
		{	
			// iterate over the space seperated arguments
			while ((sep_str = strsep(&dup_command," ")) != NULL)					
			{
				// remove whitespaces before a command if there are present
				while(*sep_str == ' ')					
					sep_str++;
				command_args[i] = strsep(&sep_str, " ");	
				i++;
			}
			
			// if current string is an empty string, assign NULL
			if(strlen(command_args[i-1])==0)
				command_args[i-1] = NULL;
			else
				command_args[i] = NULL;

			i=0;
			return command_args;
		}
		else
		{
			// assign the entire command in case of no spaces
			strcpy(command_args[0], dup_command);
			command_args[1] = NULL;
			return command_args;
		}
		
	}
	return NULL;
}
void executeCommand(char** command_arr)
{
	// This function will fork a new process to execute a command
	char **cmd_args = get_commandArgs(command_arr[0]);//only one command string
	pid_t pid=fork();
	
	//in child process
	if(pid==0)
	{
		int sig=signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		// execute the command and it's args in child
		int retval = execvp(cmd_args[0], cmd_args);
		 //execvp error code
		if (retval < 0 && !sig)     
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
		exit(0);
	}
	// in Parent Process
	else if(pid)
	{
		//wait for the child to terminate
		wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(1);
}

void executeParallelCommands(char** command_arr)
{
	// This function will run multiple commands in parallel
	int i = 0,rc=1;
	int PID[10]; 
	for(i=0;command_arr[i]!=NULL && rc!=0;i++)
	{
		if(strlen(command_arr[i])>0)
		{
			// fork process
			int pid = fork();
			rc=pid;
			PID[i]=pid;
			if (pid == 0)	
			{
				
				//we have to enable signals again for child processes	
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);	

				// get the command and arg list from given command
				char** command_args = get_commandArgs(command_arr[i]);

				int return_val = execvp(command_args[0], command_args);

				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}
				exit(0);
			}
		}
	}
	for(int j=0;j<i;j++)
	{
		waitpid(PID[j],NULL,0);
	}

}

void executeSequentialCommands(char** command_arr)
{	
	// This function will run multiple commands in sequence
	int i = 0;
	while(command_arr[i]!=NULL)
	{	
		// get the command and arg list from given command
		char* dup_command = strdup(command_arr[i]);

		// special case for "cd"
		if(strstr(dup_command,"cd")!=NULL)
		{
			get_commandArgs(command_arr[i]);
			i++;
		}
		// case where there's empty string
		else if(strlen(command_arr[i])>0)
		{

			int pid = fork(); //making child process

			// child process
			if (pid == 0)		
			{
				//we have to enable signals again for child processes
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);		

				// parsing each command
				char** command_args = get_commandArgs(command_arr[i]); 
				int return_val = execvp(command_args[0], command_args);
			
				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}	
				exit(0);
			}

			// Parent Process
			else if (pid)
			{
				// we have to enable signals again for child processes
				wait(NULL);
				i++;
			}	
			else
			{ 
				exit(1);
			}
		}
		else
		{
			i++;
		}
		
	}
}

void executeCommandRedirection(char** command_arr)
{
	// This function will run a single command with output redirected to an output file specificed by user
	// get the command and arg list from given command
	char* command = command_arr[0]; 
	char* filename = strdup(command_arr[1]);
	filename[strlen(filename)-1]='\0';
	while(*filename == ' ')			
		filename++;
	// fork 
	int pid = fork();
	// child Process
	if (pid ==0) 
	{
		close(STDOUT_FILENO);
		open(filename, O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
		// get command args to execute
		char** command_args = get_commandArgs(command);
		int retval = execvp(command_args[0], command_args);

		//execvp error code
		if (retval < 0)      
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
		exit(0);
	} 

	// Parent Process
	else if(pid)
	{
		wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(1);
}



int main()
{
	// Initial declarations
	char* input_str=NULL;
	size_t input_str_len = 64;
	//used to handle and ignore ctrl+C and ctrl+Z
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	int running=1;
	while(running)	// This loop will keep your shell running until user exits.
	{
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		char c_d[64];
		// Print the prompt in format - currentWorkingDirectory$
		getcwd(c_d, 64);
		printf("%s$",c_d); 
		// accept input with 'getline()'
		getline(&input_str,&input_str_len,stdin);
		//create an array of strings that will hold the commands seperated by delimiters
		char** command_arr;
		command_arr = (char**)malloc(sizeof(char*)*8);
		for(int i=0; i<10; i++)
			command_arr[i] = (char*)malloc(64*sizeof(char));
		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		char delimit = parseInput(input_str,command_arr); 	//store the delimiter character for using right execute function of if-block later	
		
		if(strcmp(input_str,"exit\n")==0)	// When user uses exit command.
		{
			printf("Exiting shell...\n");
			running=0;
		}
		
		if(delimit=='&')
			executeParallelCommands(command_arr);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(delimit=='#')
			executeSequentialCommands(command_arr);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(delimit=='>')
			executeCommandRedirection(command_arr);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(command_arr);		// This function is invoked when user wants to run a single commands
				
	}
	
	return 0;
}