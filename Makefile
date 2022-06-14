CC=gcc
CFlags=-g
TARGET=main.exe
strct=structures
io = io_ops
ODIR=obj
OBJS=$(ODIR)/net.o\
	 $(ODIR)/dll.o\
	 $(ODIR)/graph.o\
	 $(ODIR)/graph-topologies.o\
	 $(ODIR)/io.o\
	 $(ODIR)/ethernet.o\
	 $(ODIR)/packet.o\
	 $(ODIR)/hash.o\
	 $(ODIR)/comm_channel.o

directories : 
	@mkdir -p $(ODIR)
main.exe:main.o $(OBJS)
	$(CC) $(CFlags) main.o $(OBJS) -o main.exe -lcli -lpthread
main.o:main.c
	$(CC) $(CFlags) -c main.c -o main.o
$(ODIR)/comm_channel.o:comm_channel.c
	$(CC) $(CFlags) -c -I $(strct) comm_channel.c -o $(ODIR)/comm_channel.o
$(ODIR)/packet.o:packet.c
	$(CC) $(CFlags) -c -I $(strct) packet.c -o $(ODIR)/packet.o
$(ODIR)/ethernet.o:ethernet.c
	$(CC) $(CFlags) -c -I $(strct) ethernet.c -o $(ODIR)/ethernet.o
$(ODIR)/io.o:$(io)/io.c
	$(CC) $(CFlags) -c -I $(strct) $(io)/io.c -o $(ODIR)/io.o
$(ODIR)/graph-topologies.o:graph-topologies.c
	$(CC) $(CFlags) -c -I $(strct) graph-topologies.c -o $(ODIR)/graph-topologies.o
$(ODIR)/net.o:$(strct)/net.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/net.c -o $(ODIR)/net.o
$(ODIR)/dll.o:$(strct)/dll.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/dll.c -o $(ODIR)/dll.o
$(ODIR)/graph.o:$(strct)/graph.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/graph.c -o $(ODIR)/graph.o	
$(ODIR)/hash.o:$(strct)/hash.c
	$(CC) $(CFlags) -c -I $(strct) $(strct)/hash.c -o $(ODIR)/hash.o

all:directories $(OBJS) main.o main.exe
clean :
	rm -f $(ODIR)/*.o
	rm -rf $(ODIR)
	rm -f *.o
	rm -f *.exe