CC=gcc
CFlags=-g
TARGET=main.exe
strct=structures
ODIR=obj
OBJS=obj/net.o\
	 obj/dll.o\
	 obj/graph.o\
	 obj/graph-topologies.o

directories : 
	@mkdir -p $(ODIR)
main.exe:main.o $(OBJS)
	$(CC) $(CFlags) main.o $(OBJS) -o main.exe -lcli
main.o:main.c
	$(CC) $(CFlags) -c main.c -o main.o
obj/graph-topologies.o:graph-topologies.c
	$(CC) $(CFlags) -c -I $(strct) graph-topologies.c -o $(ODIR)/graph-topologies.o
obj/net.o:$(strct)/net.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/net.c -o $(ODIR)/net.o
obj/dll.o:$(strct)/dll.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/dll.c -o $(ODIR)/dll.o
obj/graph.o:$(strct)/graph.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/graph.c -o $(ODIR)/graph.o

all:directories $(OBJS) main.o main.exe
clean :
	rm -f $(ODIR)/*.o
	rm -rf $(ODIR)
	rm -f *.o
	rm -f *.exe