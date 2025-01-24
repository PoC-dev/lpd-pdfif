This is a filter program for providing PDF generation in *lpd* through
GhostScript and GhostPCL, with additional functionality.

It's main advantage is that it looks inside the print jobs to place the
generated PDFs in the printing user's home directory.

## License.
This document is part of lpd-pdfif, to be found on
[GitHub](https://github.com/PoC-dev/lpd-pdfif). Its content is subject to the
[CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) license, also
known as *Attribution-ShareAlike 4.0 International*. The project itself is
subject to the GNU Public License version 2.

## Rationale.
For me, the main function is to get PDFs from spooled output of older versions
of OS/400. PDF output was added considerably late, though a resource-intensive
Java application which is also not exactly easy to configure or debug if it's
not working.

If you use the same username on Linux and OS/400, the username of the print job
will also be used for determining the destination user on Linux.

See `man -l pdfif.1` for additional nicely formatted information contained in
the man page.

**WARNING!** Because lpd runs as user `lp:lp`, the resulting printer filter
binary must be installed setuid root, so it has write access to each user's
home directory! The *Makefile* takes care about that. This might have possible
security implications! I can't guarantee that the code as is free from possible
security holes allowing for privilege elevation to root! **You have been
warned!**

## Installation.
Install *lpd* per instructions for your Linux distribution. **Note:** this
might conflict with an already installed alternative printing system, such as
CUPS. I can't provide general advice how to work around this.

For easy compilation and installation of the filter program, a *Makefile* has
been provided.

### External programs required.
To do its work, the application does something very unusual for C programs:
Calling external applications. This is completely normal in the OS/400 world
but rarely done on Linux. These programs can be called from the code, depending
on the input data. `file(1)` is the only hard requirement, all others are
needed only needed for certain conversions. Make sure they're installed.

- `file(1)` to determine the type of input data
- *ghostscript* to convert PostScript data to PDF
- *ghostpcl* to convert PCL data to PDF
- *pdftk* to postprocess PDFs being generated from OS/400 PCL end up with wrong
  page orientation, depending on the OS/400 printer file being used

### Setup and configuration for *lpd*.
Example */etc/printcap* entry. Create/edit this file as shown:
```
pdfwriter:\
    :af=/var/log/lp-acct:\
    :lf=/var/log/lp-errs:\
    :sd=/var/spool/lpd/pdfwriter:\
    :lp=/var/spool/lpd/pdfwrrot/devnull:\
    :if=/usr/local/lib/lpd/pdfif:\
    :mx#0:sh:
```
To use the page rotation feature, create a symlink pointing to the compiled
program as shown, and use this in the *if*-statement.
```
lrwxrwxrwx 1 root staff     6 Nov 19  2019 ps2pdf-rotate -> pdfif
```
Of course you can configure two print spoolers: One rotated, and one plain.

Next, setup the print queue directory structure.
```
mkdir -p /var/spool/lpd/pdfwriter
chown lp:lp /var/spool/lpd/pdfwriter
chmod 2775 /var/spool/lpd/pdfwriter
mknod -m 666 /var/spool/lpd/pdfwrrot/devnull c 1 3
chown lp:lp /var/spool/lpd/pdfwrrot/devnull
```
Now you can start the spooler job.
```
lpc restart pdfwriter
```

**Note:** Depending on your distribution, *lpd* might have a startup parameter
configuration file */etc/default/lpd*, which per default denies any network
connections to lpd. If you want to submit print jobs from your network. In
addition, you must add an entry per host to */etc/hosts.lpd*.

### Run a test print.
```
man -t rmdir |lpr -P pdfwriter
```
The output PDF should appear in your home directory.

### Set default printer.
Edit */etc/environment* and add a line:
```
PRINTER=pdfwriter
```
After logging out and back in again, the `lp*` commands will use *pdfwriter* as
default printer.

## Bugs, current state.
It's basically working, but it's crude. Many values are hard coded which should
be read from a configuration file. Error recovery is minimal. Manpage isn't
installed by default. I'm releasing it in it's current state because I think it
could benefit some people anyway.

Patches to get rid of those FIXMEs in the code are welcome. Send by email to
poc@pocnet.net, provide pull requests, or ask for write access to the
repository.

----

2025-01-24 poc@pocnet.net
