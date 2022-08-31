IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -Wall -Wextra -g
GTK_CFLAGS=-`pkg-config --cflags gtk+-2.0`

ODIR=../obj

LIBS=-lpthread -lncurses -lcurl -lform -lm

CONF_OPT_GUI=--enable-gui=gtk2

_DEPS = events.h helper.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o events.o helper.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
