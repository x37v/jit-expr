first: parser

OBJS = lex.yy.o \
			 parser.tab.o

CXX = g++
CPPFLAGS = -g -std=c++11
CFLAGS = -g
LDFLAGS = -lfl


%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

lex.yy.cpp: tokens.l parser.tab.cpp #actually depends on the hpp but this will do it
	flex -o $@ $^

parser.tab.cpp: parser.y
	bison -d -o $@ $^

parser: $(OBJS)
	$(CXX) main.cpp -o $@ $(CPPFLAGS) $(LDFLAGS) $<

test: parser
	./parser examples.txt

clean:
	$(RM) -rf parser lex.yy.* *.o parser.tab.*
