all: test

objs = 
templates = Concurrent.hpp Sanitizer.hpp cshrlog.hpp refbase.hpp ThreadPool.hpp
objs = refbase.o ThreadPool.o

vpath %.hpp ../../linux:../../lockfree:../../log:../../refbase:../../threadpool
vpath %.cpp ../../refbase:../../threadpool

test : test.o $(objs)
	g++ $^ -o $@ -lpthread --std=c++17
	make clean
%.o : %.cpp
	g++ -g -c $^ -o $@ --std=c++17

clean:
	rm ./*.o

.PHONY: all
.PHONY: clean