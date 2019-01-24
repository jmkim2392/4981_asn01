/*-------------------------------------------------------------------------------------
--	SOURCE FILE: main.c - This file is the main driver for the program that imitates 
--							a Linux Terminal echo with additional translations features
--
--	PROGRAM:		main
--
--	FUNCTIONS:
--					int main(void)
--					void input_proc(int ItO_pipe[2], int ItT_pipe[2], pid_t output_pid, 
--									pid_t translate_pid);
--					void output_proc(int ItO_pipe[2], int TtO_pipe[2]);
--					void translate_proc(int ItT_pipe[2], int TtO_pipe[2]);
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		January 22, 2019
--
--	DESIGNER:		Jason Kim
--
--	PROGRAMMER:		Jason Kim
--
--	NOTES:
--	This program echo's out user's input and makes some character manipulation such as 
--	changing 'a' to 'z' and erasing characters and erasing entire lines.
--
--------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#define MSGSIZE 64

void input_proc(int ItO_pipe[2], int ItT_pipe[2], pid_t output_pid, pid_t translate_pid);
void output_proc(int ItO_pipe[2], int TtO_pipe[2]);
void translate_proc(int ItT_pipe[2], int TtO_pipe[2]);

/*-------------------------------------------------------------------------------------
--	FUNCTION:	main
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		January 22, 2019
--
--	DESIGNER:		Jason Kim
--
--	PROGRAMMER:		Jason Kim
--
--	INTERFACE:		int main(void)
--
--	RETURNS:		int
--
--	NOTES:
--	This is the main driver of the program. It initializes the pipes and forks and 
--	acts as the input process for the program. 
--------------------------------------------------------------------------------------*/
int main(void)
{
	//initialize variables
	int ItO_pipe[2];
	int ItT_pipe[2];
	int TtO_pipe[2];
	pid_t output_pid = -1;
	pid_t translate_pid = -1;

	//disable linux terminal processing
	system("stty raw igncr -echo");

	//open pipes
	if (pipe(ItO_pipe) < 0 || pipe(ItT_pipe) < 0 || pipe(TtO_pipe) < 0)
	{
		perror("Error creating pipes: ");
		exit(1);
	}

	//set the O_NDELAY flags
	if (fcntl(ItO_pipe[0], F_SETFL, O_NDELAY) < 0 || fcntl(ItT_pipe[0], F_SETFL, O_NDELAY) < 0
		|| fcntl(TtO_pipe[0], F_SETFL, O_NDELAY) < 0)
	{
		perror("Error setting pipe flags: ");
		exit(1);
	}

	//create child processes
	output_pid = fork();
	if (output_pid > 0)
	{
		translate_pid = fork();
	}

	// run functions accordingly
	if (output_pid > 0 && translate_pid > 0)
	{
		input_proc(ItO_pipe, ItT_pipe, output_pid, translate_pid);
		system("stty -raw -igncr echo");
	}
	else if (translate_pid == 0)
	{
		translate_proc(ItT_pipe, TtO_pipe);
	}
	else if (output_pid == 0)
	{
		output_proc(ItO_pipe, TtO_pipe);
	}

	exit(1);
}

/*-------------------------------------------------------------------------------------
--	FUNCTION:	input_proc
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		January 22, 2019
--
--	DESIGNER:		Jason Kim
--
--	PROGRAMMER:		Jason Kim
--
--	INTERFACE:		void input_proc(int ItO_pipe[2], int ItT_pipe[2], pid_t output_pid, 
--									pid_t translate_pid)
--						int ItO_pipe[2] - the Input - Output Pipe
--						int ItT_pipe[2] - the Input - Translate Pipe
--						pid_t output_pid - pid of the output process
--						pid_t translate_pid - pid of the translate process
--
--	RETURNS:		void
--
--	NOTES:
--	This function is called by the Input process to handle the user keystrokes.
--------------------------------------------------------------------------------------*/
void input_proc(int ItO_pipe[2], int ItT_pipe[2], pid_t output_pid, pid_t translate_pid)
{
	char input_buffer[MSGSIZE];
	char c;
	int i = 0;
	memset(input_buffer, 0, MSGSIZE);
	close(ItO_pipe[0]);
	close(ItT_pipe[0]);

	do
	{
		c = getchar();
		if (c != -1)
		{
			write(ItO_pipe[1], &c, 1);
			switch (c)
			{
			case 'E':
				i = 0;
				write(ItT_pipe[1], input_buffer, MSGSIZE);
				memset(input_buffer, 0, MSGSIZE);
				break;
			case 'T':
				kill(getpid(), SIGTERM);
				break;
			case 11:
				kill(translate_pid, SIGKILL);
				kill(output_pid, SIGKILL);
				system("stty -raw -igncr echo");
				kill(getpid(), SIGKILL);
				break;
			default:
				input_buffer[i++] = c;
			}
		};
	} while (1);
	return;
}

/*-------------------------------------------------------------------------------------
--	FUNCTION:	output_proc
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		January 22, 2019
--
--	DESIGNER:		Jason Kim
--
--	PROGRAMMER:		Jason Kim
--
--	INTERFACE:		void output_proc(int ItO_pipe[2], int TtO_pipe[2])
--						int ItO_pipe[2] - the Input - Output Pipe
--						int TtO_pipe[2] - the Translate - Output Pipe
--
--	RETURNS:		void
--
--	NOTES:
--	This function is called by the Output process to read from the two pipes and print
--	out the messages to the console.
--------------------------------------------------------------------------------------*/
void output_proc(int ItO_pipe[2], int TtO_pipe[2])
{
	char echo_buffer;
	char translated_buffer[MSGSIZE];
	close(ItO_pipe[1]);
	close(TtO_pipe[1]);

	while (1)
	{
		if (read(ItO_pipe[0], &echo_buffer, 1) > 0)
		{
			printf("%c", echo_buffer);
		}
		else
		{
			if (read(TtO_pipe[0], translated_buffer, MSGSIZE) > 0)
			{
				printf("\n\r%s\n\r", translated_buffer);
				memset(translated_buffer, 0, MSGSIZE);
			}
		}
		fflush(stdout);
	}
	return;
}

/*-------------------------------------------------------------------------------------
--	FUNCTION:	translate_proc
--
--	DATE:			January 22, 2019
--
--	REVISIONS:		January 22, 2019
--
--	DESIGNER:		Jason Kim
--
--	PROGRAMMER:		Jason Kim
--
--	INTERFACE:		void translate_proc(int ItT_pipe[2], int TtO_pipe[2])
--						int ItT_pipe[2] - the Input - Translate Pipe
--						int TtO_pipe[2] - the Translate - Output Pipe
--
--	RETURNS:		void
--
--	NOTES:
--	This function is called by the Translate process to receive sentences from the Input
--	process, translate it, and send it to Output process to be printed out
--------------------------------------------------------------------------------------*/
void translate_proc(int ItT_pipe[2], int TtO_pipe[2])
{
	char buffer[MSGSIZE];
	char translated_buffer[MSGSIZE];
	int i, j;

	close(ItT_pipe[1]);
	close(TtO_pipe[0]);

	while (1)
	{
		if (read(ItT_pipe[0], buffer, MSGSIZE) > 0)
		{
			for (i = 0, j = 0; buffer[i] != '\0'; ++i, ++j)
			{
				switch (buffer[i])
				{
				case 'a':
					translated_buffer[j] = 'z';
					break;
				case 'X':
					translated_buffer[--j] = '\0';
					--j;
					break;
				case 'K':
					memset(translated_buffer, 0, MSGSIZE);
					j = -1;
					break;
				default:
					translated_buffer[j] = buffer[i];
					break;
				}
			}
			write(TtO_pipe[1], translated_buffer, MSGSIZE);
			memset(translated_buffer, 0, MSGSIZE);
			memset(buffer, 0, MSGSIZE);
		}
	}
	return;
}
