first: parser

OBJS = scanner.o \
			 parser.o

CXX = clang++
CPPFLAGS += -g -std=c++11 -Wall
CFLAGS += -g
LDFLAGS += -lfl


%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

scanner.cpp: scanner.ll parser.cpp #actually depends on the hpp but this will do it
	flex -o $@ $^

parser.cpp: parser.yy
	bison -d -o $@ $^

parser: $(OBJS) main.cpp
	$(CXX) main.cpp -o $@ $(CPPFLAGS) $(LDFLAGS) $<

test: parser
	./parser examples.txt

clean:
	$(RM) -rf parser *.o parser.cpp parser.hpp scanner.cpp scanner.h
