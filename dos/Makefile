CC = wcc
CFLAGS = -bt=dos -zq -wx -ml -i=pdcurses -oneatx -DUSE_PDCURSES
LINK = wlink
LDFLAGS = op q sys dos
LIBCURSES = pdcurses\dos\pdcurses.lib
CSOL_SRCDIR = ..\src
RM = del

all: csol.exe

clean
	-$(RM) *.obj
	-$(RM) csol.exe

.c: $(CSOL_SRCDIR)

.c.obj: .autodepend
	$(CC) $(CFLAGS) $<

csol.exe: card.obj game.obj main.obj rc.obj theme.obj ui.obj util.obj scores.obj csv.obj
	$(LINK) $(LDFLAGS) n $@ f *.obj l $(LIBCURSES)
