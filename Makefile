#
# Copyright 2016, 2025 poc@pocnet.net
#
# This file is part of lpd-pdfif-of.
#
# This is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# It is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA 02111-1307 USA or get it at
# http://www.gnu.org/licenses/gpl.html

.PHONY: clean install install-man

DESTDIR:=/usr/local

pdfif: pdfif.o
	gcc -o $@ $<

pdfif.o: pdfif.c
	gcc -Wall -o $@ -c $<

install: pdfif install-man
	-mkdir -p $(DESTDIR)/lib/lpd
	install -o root -g lp -m 4750 $< $(DESTDIR)/lib/lpd/ps2pdf

install-man: pdfif.1
	-mkdir -p $(DESTDIR)/share/man/man1/
	install -o root -g staff -m 0644 $< $(DESTDIR)/share/man/man1/

clean:
	rm -f pdfif pdfif.o

# vi: tabstop=4 shiftwidth=4 autoindent
