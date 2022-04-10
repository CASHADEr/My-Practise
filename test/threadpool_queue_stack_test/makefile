all: test

objs = 
templates = Concurrent.hpp Sanitizer.hpp cshrlog.hpp refbase.hpp ThreadPool.hpp
objs = refbase.o ThreadPool.o

vpath %.hpp ../../linux:../../lockfree:../../log:../../refbase:../../threadpool
vpath %.cpp ../../refbase:../../threadpool

test : test.o $(objs)
	g++ $^ -o $@ -lpthread -latomic
	make clean
%.o : %.cpp
	g++ -g -c $^ -o $@ -D __cshr_debug__

clean:
	rm ./*.o

.PHONY: all
.PHONY: clean