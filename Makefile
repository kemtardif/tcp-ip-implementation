CC=gcc
CFlags=-g
strct=structures
ODIR=obj
OBJS=obj/dll.o
	 obj/graph.o

all : obj/graph.o obj/dll.o
obj/dll.o:$(strct)/dll.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/dll.c -o $(ODIR)/dll.o
obj/graph.o:$(strct)/graph.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/graph.c -o $(ODIR)/graph.o
clean :
	rm -f $(ODIR)/*.o