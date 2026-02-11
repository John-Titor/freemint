#
# Makefile for mintboot
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = mintboot

default: all-here

include $(srcdir)/MINTBOOTDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mintboottargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mintboottargets):
	$(MAKE) buildmintboot mintboot=$@

#
# multi target stuff
#

ifneq ($(mintboot),)

compile_dir = .compile_$(mintboot)
mintboottarget = _stmp_$(mintboot)
realtarget = $(mintboottarget)

$(mintboottarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmintboot: $(realtarget)
