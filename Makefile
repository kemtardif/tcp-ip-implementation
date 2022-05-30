CC=gcc
CFlags=-g
strct=structures
ODIR=obj
OBJS=obj/net.o\
	 obj/dll.o\
	 obj/graph.o\

all:directories $(OBJS)
directories : 
	@mkdir -p $(ODIR)
obj/net.o:$(strct)/net.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/net.c -o $(ODIR)/net.o
obj/dll.o:$(strct)/dll.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/dll.c -o $(ODIR)/dll.o
obj/graph.o:$(strct)/graph.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/graph.c -o $(ODIR)/graph.o
clean :
	rm -f $(ODIR)/*.o
	rm -rf $(ODIR)