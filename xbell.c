/*
 * a program to listen for X bell events and run a user defined program
 * when it is caught.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <sys/types.h>
#include <X11/XKBlib.h>

/*
 * maximum number of arguments we will try and call when
 * launching the user defined program
 */
#define ARG_LIMIT 20

#define BUFF_SIZE 512

char *program = "sh -c /home/daniel_j/programming/bash/beep.sh";

Display *display;
XEvent xevent;
XkbEvent *xkbevent;
int xkbeventcode; /* stores the event code for the xkb extension */

int
forkexecute()
{
	pid_t pid = fork();
	if (pid == 0)
	{
		// child process, we don't want to ignore signals
		signal(SIGCHLD, SIG_DFL);
		/*
		 * we don't want std{out,err} to be associated with the terminal,
		 * but we also don't want to close it to avoid the file descriptors
		 * being re-used potentially leading to problems, so reopen them to /dev/null
		 */
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
		char *args[ARG_LIMIT];
		char *buff = malloc(BUFF_SIZE);
		if (buff == NULL)
		{
			perror("malloc");
			return -1;
		}
		/*
		 * the program we're calling might have arguments,
		 * so we tokenise the string and add each part to an array
		 * that we will use in execvp
		 */
		strncpy(buff, program, BUFF_SIZE-1);
		char *t = strtok(buff, " ");
		int z = 0;
		while (t != NULL && z < ARG_LIMIT-1) // save a position for NULL
		{
			args[z] = t;
			t = strtok(NULL, " ");
			z++;
		}
		args[z] = (char *)0;
		execvp(args[0], args);
		_exit(1);
	}
	else if (pid == -1)
	{
		perror("fork");
		return -1;
	}
	return 1;
}

void
bellevent(void)
{
	/*
	 * a bell event was caught, execute the user defined program
	 */
	int ret = forkexecute();
	if (ret != 1)
		fprintf(stderr, "forkexecute() failed");
}

int
main(void)
{
	// ignore child processes
	signal(SIGCHLD, SIG_IGN);
	// ignore return values for now
	display = XkbOpenDisplay(NULL, &xkbeventcode, NULL, NULL, NULL, NULL);
	if (!display)
	{
		fprintf(stderr, "Cannot open display.\n");
		exit(1);
	}

	XkbSelectEvents(display, XkbUseCoreKbd, XkbBellNotifyMask, XkbBellNotifyMask);


	while (1)
	{
		XNextEvent(display, &xevent);
		if (xevent.type == xkbeventcode)
		{
			xkbevent = (XkbEvent *)&xevent;
			if (xkbevent->any.xkb_type == XkbBellNotify)
			{
				bellevent();
			}
		}
	}
	return 0;
}
