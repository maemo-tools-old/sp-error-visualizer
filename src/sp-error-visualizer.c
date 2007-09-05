/*
 * This file is part of sp-error-visualizer
 *
 * Copyright (C) 2006 Nokia Corporation. 
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/*
 * reads stdin or syslog socket and shows data as banner messages.
 * Intended for showing interesting syslog messages on screen 
 * when they happen, for example GLIB criticals and such
 *
 * Compile:
 gcc `pkg-config --cflags --libs libosso` -g -Wall -O2 -o \ 
 sp-error-visualizer sp-error-visualizer.c
 *
 * usage example on device (your syslog file location may vary)
 tail -f /var/log/syslog | ./sp-error-visualizer -f syslog_patterns &
 tail -f /var/log/syslog | ./sp-error-visualizer GLIB &
 ./sp-error-visualizer -s -f syslog_patterns &
 * usage exmple on SDK:
 tail -f /var/log/syslog | run-standalone.sh ./sp-error-visualizer -f syslog_patterns GLIB &
 tail -f /var/log/syslog | run-standalone.sh ./sp-error-visualizer GLIB &
 *
 *
 Useful syslog_patterns compiled by Eero: 
 (put this block into file and use it via -f argument)
 Note that: 
  - whitespaces are meaningful, so keep them also at end of lines!
  - #comment lines and empty lines are OK (skipped)

  *************** START OF EXAMPLE syslog_patterns file **************
# Kernel (reported) issues
 SysRq 
 Oops: 
Out of Memory: Kill
lowmem: denying memory

# DSP issues
mbox: Illegal seq bit
omapdsp: poll error
mbx: ERR

# Connectivity issues
cx3110x ERROR
TX dropped
We haven't got a READY interrupt
We haven't got a WR_READY interrupt

# DSME reported issues
spawning too fast -> reset
exited with RESET
exited and restarted
exited with signal: 

# Maemo-launcher reported issues
exited with return value: 
exited due to signal

# Glib reported issues
GLIB WARNING
GLIB CRITICAL
GLIB ERROR
  *************** END OF EXAMPLE syslog_patterns file **************
*/

/* Includes */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <fcntl.h>
#include <libosso.h>

#define SYSLOG_MONITOR_DBUS_SERVICE "syslog_monitor"
#define MAXMSG (256)
#define MAXPATTERNS (256)

char buf[MAXMSG + 1];
char prev[MAXMSG + 1];
char *pattern[MAXPATTERNS];
int patnum = 0;

void read_patterns(char *fname)
{
    int len;
    FILE *f;
    f = fopen(fname, "r");
    if (f == NULL) {
	perror("Can not open patterns file:");
	exit(1);
    }
    patnum = 0;
    while (fgets(buf, MAXMSG, f)) {
	if (patnum >= MAXPATTERNS) {
	    return;
	}
	if (buf[0] == '#') {
	    continue;
	}
	len = strlen(buf);
	if (len <= 1) {
	    /* just newline */
	    continue;
	}
	if (len > 0 && buf[len - 1] == '\n') {
	    buf[len - 1] = 0;
	}
	pattern[patnum] = (char *) malloc(len + 1);
	if (!pattern[patnum]) {
	    g_print("read_patterns: Can not allocate memory\n");
	    exit(1);
	}
	strcpy(pattern[patnum], buf);
	g_print("pattern[%3d]: [%s]\n", patnum, pattern[patnum]);
	patnum++;
    }
    fclose(f);
    g_print("read_patterns: processed %d patterns from file %s\n", patnum,
	    fname);
}

int main(int argc, char *argv[])
{
    osso_context_t *osso_context;
    int numspaces, len;
    char *patternfile = NULL, *logfile = NULL;
    char *p, *end;
    int pn, pattern_found;
    int slog_socket = -1, readfromfile = 0, inotify_fd = -1, logwatch = -1;
    int optchar;

    struct sockaddr slog_socket_addr;
    fd_set fds;

    while ((optchar = getopt(argc, argv, "sm:f:")) != -1) {
	switch (optchar) {

	case 's':

	    /* The check below might fail for other reasons than the
	       non-existence of the socket. However, in that case reading
	       from the logfile via piping to stdin probably fails too */

	    if ((access(_PATH_LOG, F_OK) == 0)) {
		g_print
		    ("Won't read from socket as syslog appears to exist.\n");
		g_print("See documentation for more details.\n");
		return 1;
	    }

	    /* Ensure that previous syslog sockets are not dangling around */

	    unlink(_PATH_LOG);

	    slog_socket_addr.sa_family = AF_UNIX;
	    strncpy(slog_socket_addr.sa_data, _PATH_LOG,
		    sizeof(slog_socket_addr.sa_data));
	    slog_socket = socket(AF_UNIX, SOCK_DGRAM, 0);

	    if (bind(slog_socket, &slog_socket_addr,
		     sizeof(slog_socket_addr.sa_family) +
		     strlen(slog_socket_addr.sa_data)) == -1) {
		g_warning("Syslog socket invocation failed!\n");
		perror("Reason was ");
		return (1);
	    }

	    /* Setup permissions to allow everybody r/w access to log */
	    if (chmod(_PATH_LOG, 0666) < 0) {
		g_warning("Could not setup syslog socket permissions\n");
		return 1;
	    }

	    break;

	case 'f':
	    patternfile = (char *) optarg;
	    read_patterns(patternfile);
	    readfromfile = 1;
	    break;

	case 'm':
	  logfile = (char *) optarg;
	  /* if we're not using syslog socket (i.e. we're getting data from
	     standard input, we'll have to initialize an inotify queue for
	     monitoring log rotation as a workaround for the current
	     limitations in the busybox tail. */
	  
	  if (slog_socket < 0) {
	    
	    inotify_fd = inotify_init();
	    if (inotify_fd < 0) {
	      perror("inotify_init failed: ");
	      g_print("Aborting...");
	    }

	    g_print("Starting inotify monitoring for file %s\n", logfile);
	    logwatch = inotify_add_watch(inotify_fd, logfile,
					 IN_MOVE_SELF | IN_DELETE_SELF);
	    if (logwatch < 0) {
	      perror("inotify_add_watch failed: ");
	    }
	    break;
	  }
	  else {
	    g_print("-m does not make sense in syslogd replacement mode.\n");
	    close(slog_socket);
	    return 1;
	  }

	default:
	    g_print
		("Please look at the documentation for usage information\n");
	    return 1;
	}
    }

    if (!readfromfile && optind < argc) {
	for (; patnum < (argc - 1) && patnum < MAXPATTERNS; patnum++) {
	    pattern[patnum] = argv[optind + patnum];
	    g_print("pattern[%3d]: [%s]\n", patnum, pattern[patnum]);
	}
    }

    osso_context = osso_initialize(SYSLOG_MONITOR_DBUS_SERVICE,
				   "0.1", FALSE, NULL);
    if (!osso_context) {
	g_print("Error doing osso_initialize()\n");
	return 1;
    }

    /* Using fdopen and fcntl/setsockopts might allow using just fgets
       for both socket and piped input, but the below approach is
       probably safer */

    while (1) {

	memset(&buf, '\0', sizeof(buf));

	if (slog_socket < 0) {

	  FD_ZERO(&fds);
	  FD_SET(0, &fds);
	  FD_SET(inotify_fd, &fds);

	  if (select(inotify_fd + 1, &fds, NULL, NULL, NULL) < 0) {
	    g_print("Select failed. Exiting.\n");
	    close(inotify_fd);
	    return 1;
	  }
	  else if (FD_ISSET(inotify_fd, &fds)) {
	    /* We don't really have to read the actual event, as
	       anything that triggers our rule means that we need
	       to exit anyway. */
	    /* FIXME: Other cleanup needed? */
	    close(inotify_fd);
	    exit(0);
	  }
	  else if (FD_ISSET(0, &fds)) {
	    if (!fgets(buf, MAXMSG, stdin)) {
	      break;
 	    }
	  }
	      
	} else {
	    FD_ZERO(&fds);
	    FD_SET(slog_socket, &fds);
	    if (select(slog_socket + 1, &fds, NULL, NULL, NULL) < 0) {
		g_print("Select failed. Exiting.\n");
		close(slog_socket);
		return 1;
	    }
	    if (FD_ISSET(slog_socket, &fds)) {
		int ret;
		ret = recv(slog_socket, buf, MAXMSG - 1, 0);
		if (ret < 0) {
		    g_print
			("Reading from the syslog socket failed. Exiting.\n");
		    close(slog_socket);
		    return 1;
		}
	    }
	}
	p = buf;
	end = buf + MAXMSG;

	/*
	 * poor man parser to ignore prefix of typical syslog line:
	 *
	 May 16 12:53:44 Nokia-N800-19 iap_conndlg 1.3.51[1915]: NameownerChanged(:1.1278, , :1.1278)
	 *
	 * we use part starting after 4th space
	 */
	for (numspaces = 0; p < end && numspaces < 4; p++) {
	    if (*p == ' ') {
		numspaces++;
	    }
	}
	if (!strncmp(p, prev, MAXMSG)) {
	    /*
	     * repeating msg
	     */
	    continue;
	}
	for (pattern_found = pn = 0; pn < patnum; pn++) {
	    if (strstr(p, pattern[pn])) {
		pattern_found = 1;
		break;
	    }
	}
	if (patnum && !pattern_found) {
	    continue;
	}

	strncpy(prev, p, MAXMSG);
	len = strlen(p);
	if (p[len - 1] == '\n') {
	    p[len - 1] = 0;
	}
	osso_system_note_infoprint(osso_context, p, NULL);
	}

    osso_deinitialize(osso_context);
    if (inotify_fd > 0) {
      close(inotify_fd);
      }
    return 0;
    }
