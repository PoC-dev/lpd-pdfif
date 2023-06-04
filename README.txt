What is it?
===========
It is a converter for providing PDF-services in lpd through GhostScript and
GhostPCL, and licensed under the terms of GPL v2 or later.

It's main advantage is that it looks inside the print jobs to place the
generated PDFs in the printing user's home directory.

WARNING! Because lpd runs as user lp:lp, the resulting printer filter binary
must be installed setuid root, so it has write access to each user's home
directory! The Makefile takes care about that. This might have possible
security implications! I can't guarantee that the code as is doesn't contain
possible security holes allowing for privilege elevation to root! You have been
warned!

For me, it's main function I use is to get PDFs from spooled output of older
versions of OS/400. PDF output was added considerably late, though a
resource-intensive Java application which is also not exactly easy to configure
or debug if it's not working. If you use the same username on Linux and OS/400
(and, currently in UPPERCASE which is ugly), the username of the print job will
also be inherited and used for the destination user on Linux.

See man -l pdfif.1 for nicely formatted information contained in the man page.

Current state
=============
It's basically working, but it's crude. Many values are hard coded which should
be read from a configuration file. Error recovery is minimal. Manpage isn't
installed by default. Debian packaging hasn't been tested at all. I'm releasing
it in it's current state because I think it could benefit skilled people anyway.

Patches to get rid of those FIXMEs in the code are welcome. Send by email to
poc@pocnet.net, or ask for write access to the repository.

External programs required
==========================
To do it's work, I do something very unusual: Calling external applications.
This is completely usual in the OS/400 world but rarely done on Linux.

These programs can be called from the code. File is the only hard requirement,
all others are needed only needed for certain conversions.
- file
- ghostscript (only for PostScript input)
- gpcl (only for PCL input)
- pdftk (only for certain PCL input)

Example /etc/printcap entry
===========================
pdfwriter:\
        :af=/var/log/lp-acct:\
        :lf=/var/log/lp-errs:\
        :sd=/var/spool/lpd/pdfwriter:\
        :if=/usr/local/lib/lpd/pdfif:\
        :lp=/dev/null:\
        :mx#0:sh:

Setup print queue
=================
mkdir -p /var/spool/lpd/pdfwriter
chown lp:lp /var/spool/lpd/pdfwriter
chmod 2775 /var/spool/lpd/pdfwriter

lpc restart pdfwriter

Do a test print
===============
man -t rmdir |lpr -P pdfwriter

Set default printer
===================
Edit /etc/environment and add a line:
PRINTER=pdfwriter

After logging out and back in again, the lp* commands will use "pdfwriter" as
default printer.

$Id: README.txt,v 1.3 2023/06/04 20:31:11 poc Exp $
