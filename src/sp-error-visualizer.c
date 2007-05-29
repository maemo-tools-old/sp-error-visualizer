/*
 * read stdin and show as banner messages.
 * Intended for showing interesting syslog messages on screen 
 * when they happen, for example GLIB criticals and such
 *
 * Compile:
 gcc `pkg-config --cflags --libs libosso` -g -Wall -O2 -o show_banner show_banner.c
 *
 * usage example on device (your syslog file location may vary)
 tail -f /var/ftd-log/syslog | ./show_banner -f syslog_patterns &
 tail -f /var/ftd-log/syslog | ./show_banner GLIB &
 * usage exmple on SDK:
 tail -f /var/ftd-log/syslog | run-standalone.sh  -f syslog_patterns GLIB &
 tail -f /var/ftd-log/syslog | run-standalone.sh ./show_banner GLIB &
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>

#define SYSLOG_MONITOR_DBUS_SERVICE "syslog_monitor"
#define MAXMSG (256)
#define MAXPATTERNS (256)

char buf[MAXMSG + 1];
char prev[MAXMSG + 1];
char *pattern[MAXPATTERNS];
int patnum = 0;
 
void read_patterns(char * fname)
{
    int len;
    FILE *f;
    f = fopen(fname, "r");
    if ( f == NULL) { 
	perror("Can not open patterns file:");
	exit(1);
    }
    patnum = 0;
    while(fgets(buf, MAXMSG, f)) {
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
	pattern[patnum] = (char *)malloc(len + 1);
	if(!pattern[patnum]) {
	    g_print("read_patterns: Can not allocate memory\n");
	    exit(1);
	}
	strcpy(pattern[patnum], buf);
	g_print("pattern[%3d]: [%s]\n", patnum, pattern[patnum]);
	patnum++;
    }
    fclose(f);
    g_print("read_patterns: processed %d patterns from file %s\n", patnum, fname);
}

int main(int argc, char *argv[])
{
    osso_context_t *osso_context;
    int numspaces, len;
    char *p, *end;
    int pn, pattern_found;

    if (argc > 1) {
	if (!strcmp(argv[1] , "-f")) {
	    if (argc != 3) {
		g_print("if giving pattern filename, must use exactly two arguments: %s -f FILENAME\n", argv[0]);
		return 1;
	    }
	    read_patterns(argv[2]);
	} else {
	    for (;patnum < (argc - 1) && patnum < MAXPATTERNS; patnum++) {
		pattern[patnum] = argv[patnum + 1];
		g_print("pattern[%3d]: [%s]\n", patnum, pattern[patnum]);
	    }
	}
    }

    osso_context = osso_initialize(SYSLOG_MONITOR_DBUS_SERVICE,
				   "0.1", FALSE, NULL);
    if (!osso_context) {
	g_print("Error doing osso_initialize()\n");
	return 1;
    }

    while (1) {
	if (!fgets(buf, MAXMSG, stdin)) {
	    break;
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
	    if(strstr(p, pattern[pn])) {
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

    osso_deinitialize (osso_context);
    return 0;
}

