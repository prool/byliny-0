# makefile for debian 6.0. by prool (www.prool.kharkov.org)
# gcc (Debian 4.4.5-8) 4.4.5

# tested under Debian, Centos, Ubuntu

# C compiler to use
CC = g++

# Path to cxref utility
CXREF = cxref

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -g -fpermissive

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)
#CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE) -DPROOLCRYPT

#LIBS =  -lsocket -lnsl
LIBS =  -lcrypt

OBJFILES = act.comm.o act.informative.o act.item.o act.movement.o \
	act.offensive.o act.other.o act.social.o act.wizard.o alias.o ban.o \
	boards.o castle.o class.o comm.o config.o constants.o db.o fight.o \
	graph.o handler.o house.o interpreter.o limits.o magic.o mail.o \
	mobact.o modify.o objsave.o olc.o random.o shop.o spec_assign.o \
	spec_procs.o spell_parser.o spells.o utils.o weather.o \
	redit.o oedit.o zedit.o medit.o sedit.o \
	dg_comm.o dg_db_scripts.o dg_event.o dg_handler.o dg_mobcmd.o \
	dg_misc.o dg_objcmd.o dg_scripts.o dg_triggers.o dg_wldcmd.o dg_olc.o \
	prool.o

CXREF_FILES = act.comm.c act.informative.c act.item.c act.movement.c \
	act.offensive.c act.other.c act.social.c act.wizard.c alias.c ban.c \
	boards.c castle.c class.c comm.c config.c constants.c db.c fight.c \
	graph.c handler.c house.c interpreter.c limits.c magic.c mail.c \
	mobact.c modify.c objsave.c olc.c random.c shop.c spec_assign.c\
	spec_procs.c spell_parser.c spells.c utils.c weather.c \
	redit.c oedit.c zedit.c medit.c sedit.c \
	dg_comm.c dg_db_scripts.c dg_event.c dg_handler.c dg_mobcmd.c \
	dg_misc.c dg_objcmd.c dg_scripts.c dg_triggers.c dg_wldcmd.c dg_olc.c \
	prool.c

default: all

all:
	$(MAKE) $(BINDIR)/circle
#	$(MAKE) utils

.accepted:
	@./licheck less

utils:
	(cd util; $(MAKE) all)
circle:
	$(MAKE) $(BINDIR)/circle
depend:
	$(CC) -MM $(CXREF_FILES) > .depend
	@echo "Done building dependencies."

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

clean:
	rm -f *.o
ref:
#
# Create the cross reference files
# Note, this is not meant to be used unless you've installed cxref...
#
	@for file in $(CXREF_FILES) ; do \
	  echo Cross referencing $$file ; \
	  $(CXREF) -D__CXREF__ -xref -Odoc -Ncircle $$file ; \
	done
#
# Create the source files using cxref
#
	@for file in $(CXREF_FILES) ; do \
	   echo Documenting $$file ; \
	   ( cd . ; $(CXREF) -D__CXREF__ -warn-xref -xref -Odoc -Ncircle -html $$file ) ; \
	   rm -f $(DOCS) ; \
	done
#
# Create the index using cxref
#
	@echo Indexing
	@( cd . ; $(CXREF) -D__CXREF__ -index-all -Odoc -Ncircle -html )
	@rm -f $(DOCS)
#
# Make html files for the .h files
#
	@echo Creating .h.html files...
	@for file in *.h ; do \
	  echo $$file ; \
	  cat /home/jelson/mud/htmlh-head $$file /home/jelson/mud/htmlh-tail > doc/$$file.html ; \
	done
# Copy over to the html directory
	cp doc/*.html /home/jelson/public_html/circle/cdp/cxref
	chmod 644 /home/jelson/public_html/circle/cdp/cxref/*.html

# Dependencies for the object files (automagically generated with
# gcc -MM)

.cpp.o:
	$(CC) -c $(CFLAGS) -o $@ $<

include .depend
