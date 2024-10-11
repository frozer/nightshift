AUTOMAKE_OPTIONS = foreign
SUBDIRS = libdozor dozord tools
DISTDIR = $(top_builddir)/dist

# Override the 'distdir' target
dist-hook:
	@mkdir -p $(DISTDIR)
	@mv $(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(DISTDIR)/
  
clean-local:
  @echo: This will be run by "make clean"