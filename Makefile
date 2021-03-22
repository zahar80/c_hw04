CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g -DNDEBUG
RM = rm -rf
MKDIR = mkdir -p
COPY = cp
CD = cd

SRCDIR = src
DISTDIR = dist
EXECFILE = main

build: clean mkdir compile

compile:
	$(CC) $(CFLAGS) $(SRCDIR)/main.c $(SRCDIR)/http.c $(SRCDIR)/parson.c `curl-config --libs` -o $(DISTDIR)/$(EXECFILE)

clean:
	$(RM) $(DISTDIR)

mkdir:
	$(MKDIR) $(DISTDIR)

check-mem-leaks:
	$(CD) $(DISTDIR) && valgrind --leak-check=full --track-origins=yes  ./$(EXECFILE) moscow
