AUTOMAKE_OPTIONS = foreign
SUBDIRS = liblogger libdozor dozord tools
DISTDIR = $(top_builddir)/dist
ACLOCAL_AMFLAGS = -I m4

# Override the 'distdir' target
#dist-hook:
#	@mkdir -p $(DISTDIR)
#	@mv $(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(DISTDIR)/
  
clean-local:
  @echo: This will be run by "make clean"