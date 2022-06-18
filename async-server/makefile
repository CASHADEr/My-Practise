all: test

objs = TcpServer.o io_uring_handle.cpp EventLoop.o IPV4.o 
vpath %.cpp vpath %.cpp

test : main.o $(objs)
	g++ $^ -o $@ -luring
	# make clean
%.o : %.cpp
	g++ -g -c $^ -o $@ 

clean:
	rm ./*.o

.PHONY: all
.PHONY: clean