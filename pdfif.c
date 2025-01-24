/*
 * Copyright 2016, 2025 poc@pocnet.net
 *
 * This file is part of lpd-pdfif.
 *
 * lpd-pdfif is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * lpd-pdfif is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * lpd-pdfif; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA or get it at
 * http://www.gnu.org/licenses/gpl.html
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <libgen.h>
#include <errno.h>
#include <signal.h>

/* For nicer syslog output */
#define SYSLOGNAME "pdfif"
#define SYSLOG_FACILITY LOG_LPR

/* Various Buffers */
#define BUFSIZE 10240

/* This user receives output if user cannot be determined */
/* FIXME: Make dynamic in configuration file. */
#define DFT_USER "poc"

/* Configuration file */
#define CONFFILE "/etc/pdfifrc"

/* Enable debugging messages by defining DEBUG */
#undef DEBUG

/* --------------------------------------------------------------------------*/
/* Convert string in *buf to lower case */

int str2lcase(char *buf) {
    unsigned int    i;

    for (i = 0; buf[i]; i++) {
        buf[i] = tolower(buf[i]);
    }

    return(0);
}

/* --------------------------------------------------------------------------*/

/* FIXME: from printcap(5): we must not ignore SIGINT. */

int main(int argc, char *argv[]) {
    char            buf[BUFSIZE], popen_readbuf[BUFSIZE], extprog_call[BUFSIZE],
                    dstfile_buf[BUFSIZE];
    int             retval = 0, is_pcl;
    char            *tmpfile, *pdffile, *lpuser, *lphost, *lpfile,
                    optchar;
    FILE            *tmpfd, *filecmdfp;
    struct passwd   *dstUsrEntry;


    /* Initialize stuff */
    memset(buf,           '\0', BUFSIZE);
    memset(popen_readbuf, '\0', BUFSIZE);
    memset(extprog_call,  '\0', BUFSIZE);
    memset(dstfile_buf,   '\0', BUFSIZE);
    lpuser = lphost = lpfile = NULL;


    /* Parse arguments (we get these from lpd) */
    while ((optchar = getopt (argc, argv, "cw:l:i:n:h:j:")) != -1)
    switch (optchar) {
        case 'c':
            break;  /* Ignored, not utilized */
        case 'w':
            break;  /* Ignored, not utilized */
        case 'l':
            break;  /* Ignored, not utilized */
        case 'i':
            break;  /* Ignored, not utilized */
        case 'j':
            str2lcase(optarg);
            lpfile = optarg;
            break;
        case 'n':
            str2lcase(optarg);
            lpuser = optarg;
            break;
        case 'h':
            str2lcase(optarg);
            lphost = optarg;
            break;
        case '?':
            if (optopt == 'w' || optopt == 'l' || optopt == 'i'
                    || optopt == 'n' || optopt == 'h' ) {
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                syslog(LOG_ERR, "Option -%c requires an argument.\n",
                    optopt);
            } else if (isprint (optopt)) {
                fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                syslog(LOG_ERR, "Unknown option '-%c'.\n", optopt);
            } else {
                fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                syslog(LOG_ERR, "Unknown option character '\\x%x'.\n",
                    optopt);
            }
            return(1);
        default:
            abort();
    }


    /* FIXME: Open Global config file and read stuff there into an array. */


    /* Lookup user: given with -n, DFT_USER, root */
    /* FIXME: OS/400 uses all UPPERCASE user names. Provide means to match users case-insensitive. */
    if ( lpuser == NULL ) {
        syslog(LOG_ERR, "Need username to be given with -n.\n");
        exit(1);
    } else {
        dstUsrEntry = getpwnam(lpuser);
        if ( dstUsrEntry == NULL ) {
            dstUsrEntry = getpwnam(DFT_USER);
            if ( dstUsrEntry == NULL ) {
                dstUsrEntry = getpwnam("root");
                if ( dstUsrEntry == NULL ) {
                    syslog(LOG_ERR, "Error resolving '%s' or '%s' or '%s' to local user.\n",
                        lpuser, DFT_USER, "root");
                    exit(1);
                }
            }
        }
    }

    /* Drop privileges. Do it properly!
     * https://wiki.sei.cmu.edu/confluence/display/c/POS36-C.+Observe+correct+revocation+order+while+relinquishing+privileges
     * http://blog.boramalper.org/programming/dropping-root-privileges-permanently-on-linux-in-c
     */
    if ( setgid(dstUsrEntry->pw_gid) != 0 ) {
        syslog(LOG_WARNING, "Could not setgid() to %d.\n", dstUsrEntry->pw_gid);
    }
    if ( setuid(dstUsrEntry->pw_uid) != 0 ) {
        syslog(LOG_WARNING, "Could not seugid() to %d.\n", dstUsrEntry->pw_uid);
    }
    /* FIXME: properly handle auxiliary groups! */


    /* Create tempfiles */
    /* FIXME: Replace tmpnam/tempnam with mkstemp */
    /* http://stackoverflow.com/questions/1188757/getting-filename-from-file-descriptor-in-c */
    /* http://www.linuxquestions.org/questions/programming-9/getting-file-path-from-file-pointer-in-c-245415/ */
    tmpfile = tmpnam(NULL);
    pdffile = tempnam("/tmp", "pdf");

    if ( tmpfile == NULL || pdffile == NULL ) {
        syslog(LOG_ERR,
            "Failed to create temp files in /tmp. Check free inodes and space.\n");
        exit(1);
    }

    /* Write lpr output to temp file for later processing with gs */
    errno = 0;
    tmpfd = fopen(tmpfile, "w");
    if (tmpfd == NULL) {
        syslog(LOG_ERR, "Failed to open temp file for writing: %s\n",
            strerror(errno));
        unlink(tmpfile);
        exit(1);
    }
    errno = 0;
    while ( fgets(buf, BUFSIZE-1, stdin) != NULL ) {
        if ( fputs(buf, tmpfd) == EOF ) {
            syslog(LOG_ERR, "Error writing temp file: %s\n",
                strerror(errno));
            unlink(tmpfile);
            exit(1);
        }
    }
    fclose(tmpfd);
    memset(buf, '\0', BUFSIZE);

    /* FIXME: Check if file(1) program exists/may be exec'd
     *        and if not pass input unconverted.
     */

    /* What kind of data did we collect so far? */
    snprintf(extprog_call, BUFSIZE-1, "/usr/bin/file -b %s\n", tmpfile);
    errno = 0;
    filecmdfp = popen(extprog_call, "r");
    if (filecmdfp == NULL) {
        syslog(LOG_ERR, "Failed to run file command: %s\n", strerror(errno));
        /* FIXME: Don't just exit but copy the contents to final dest
         *        so the job is not lost - set some kind of error flag? */
        exit(1);
    }
    fgets(popen_readbuf, BUFSIZE-1, filecmdfp);
    pclose(filecmdfp);
    memset(extprog_call, '\0', BUFSIZE);

    /* FIXME: Check if gs/gpcl programs exist/may be exec'd
     *        and if not pass input unconverted.
     */

    /* Call what's needed for converting the source data. */
    if ( strncmp(popen_readbuf, "PostScript document text", 24) == 0 || 
            strncmp(popen_readbuf, "HP PCL printer data", 19) == 0 ) {

        /* Decide what to call, then use common aftermath */
        if ( strncmp(popen_readbuf, "PostScript document text", 24) == 0 ) {
            is_pcl = 0;
            snprintf(extprog_call, BUFSIZE-1,
                "/usr/bin/gs -P- -dSAFER -q -P- -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile=%s -P- -dSAFER -c .setpdfwrite -f %s\n",
                pdffile, tmpfile);
        } else if ( strncmp(popen_readbuf, "HP PCL printer data", 19) == 0 ) {
            is_pcl = 1;
            snprintf(extprog_call, BUFSIZE-1,
                "/usr/local/bin/gpcl -dSAFER -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile=%s %s\n",
                pdffile, tmpfile);
        }


        /* Common routines for both: Call conversion program */
        /* FIXME: If this fails, pass input unconverted. */
        errno = 0;
        if ( system(extprog_call) != 0 ) {
            syslog(LOG_WARNING, "Failed to run command line '%s': %s\n",
                extprog_call, strerror(errno));
        }

        /* Cleanup */
        memset(buf,           '\0', BUFSIZE);
        memset(popen_readbuf, '\0', BUFSIZE);
        memset(extprog_call,  '\0', BUFSIZE);


        /* PDF must be rotated when input was PCL via standard PRTF on OS/400. */
        /* FIXME: This is a very specific requirement and should be made configurable. */
        if ( (strncmp(argv[0], "ps2pdf-rotate", 13)) == 0 ) {
            snprintf(extprog_call, BUFSIZE-1,
                "pdftk %s cat 1-endwest output %s.pdf; cat %s.pdf > %s; rm -f %s.pdf",
                pdffile, pdffile, pdffile, pdffile, pdffile);
            errno = 0;
            retval = system(extprog_call);
            if ( retval != 0 ) {
                syslog(LOG_WARNING, "system(%s) returned %d, %s\n",
                    extprog_call, retval, strerror(errno));
            }
            memset(extprog_call, '\0', BUFSIZE);
        }


        /* FIXME: If we still have no dstUsrEntry->pw_dir, what to do? */

        /* FIXME: Open CONFFILE and then~/.pdfifrc (as override), read and
         *        check what to do: Empty/not existant: Copy to local dir as
         *        usual. Contains Mailadress: Send to mailadresss as
         *        MIME-Mail, maybe also with external program, mutt or simimar.
         */

        /* Build the destination path-and-name
         * FIXME: disabled because we need unique file names.
        if ( lpfile != NULL && strncmp(lpfile, "stdin", 6) != 0 ) {
            snprintf(buf, BUFSIZE-1, "%s/%s.pdf", dstUsrEntry->pw_dir, basename(lpfile));
        } else {
        }
        */ 

        /* Work on a copy, so we don't interfere with original data */
        strncpy(dstfile_buf, pdffile, BUFSIZE-1);
        snprintf(buf, BUFSIZE-1, "%s/%s.pdf",
            dstUsrEntry->pw_dir, basename(dstfile_buf));


        /* Indicate we were running. */
        if ( is_pcl == 0 ) {
            syslog(LOG_INFO, "PostScript Job '%s' for '%s' converted to '%s'.\n",
                lpfile, lpuser, buf);
        } else {
            syslog(LOG_INFO, "PCL Job '%s' for '%s' converted to '%s'.\n",
                lpfile, lpuser, buf);
        }


        /* Try to move file to the user's home directory  */
        /* FIXME: What if there's a named file already existing? => Append numbers and count upwards? Or create new tmp name file? */
        errno = 0;
        retval = rename(pdffile, buf);
        if ( retval != 0 ) {
            #ifdef DEBUG
            syslog(LOG_DEBUG, "could not move '%s' to '%s': %s\n",
                pdffile, buf, strerror(errno));
            #endif

            snprintf(extprog_call, BUFSIZE-1, "/bin/cp -a '%s' '%s'",
                pdffile, buf);

            errno = 0;
            retval = system(extprog_call);
            if ( retval != 0 ) {
                syslog(LOG_WARNING, "system(%s) returned %d, %s\n",
                    extprog_call, retval, strerror(errno));
            } else {
                errno = 0;
                retval = unlink(pdffile);
                if ( retval != 0 ) {
                    syslog(LOG_WARNING, "unlink(%s) returned %d, %s\n",
                        pdffile, retval, strerror(errno));
                }
            }
        }
    } else {
        syslog(LOG_NOTICE, "Unknown file type %s, ignoring.\n",
            popen_readbuf);
    }

    /* Cleanup */
    memset(buf,           '\0', BUFSIZE);
    memset(popen_readbuf, '\0', BUFSIZE);
    memset(extprog_call,  '\0', BUFSIZE);
    memset(dstfile_buf,   '\0', BUFSIZE);

    errno = 0;
    retval = unlink(tmpfile);
    if ( retval != 0 ) {
        syslog(LOG_WARNING, "Removing tmpfile with unlink(%s) returned %d, %s\n",
            tmpfile, retval, strerror(errno));
    }

    return(retval);
}

/* vi: tabstop=4 shiftwidth=4 autoindent expandtab
 *EOF */
