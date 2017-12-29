OBJS = lex.yy.o main.o
CXX = clang++
CPPFLAGS = -g -std=c++11
LDFLAGS = -lfl

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

lex.yy.cpp: tokens.l
	flex -o $@ $^

parser: $(OBJS)
	$(CXX) -o $@ $(CPPFLAGS) $(LDFLAGS) $<

test: parser
	cat examples.txt | ./parser

clean:
	$(RM) -rf parser lex.yy.cpp
