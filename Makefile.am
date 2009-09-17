SUBDIRS = autotools src data

AUTOMAKE_OPTIONS = foreign

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing

ACLOCAL_AMFLAGS = -I autotools

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`

