OBJS = lex.yy.o
CXX = g++
CPPFLAGS = -g -std=c++11
LDFLAGS = -lfl

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) -o $@ $<

lex.yy.cpp: tokens.l
	flex -o $@ $^

parser: $(OBJS)
	$(CXX) main.cpp -o $@ $(CPPFLAGS) $(LDFLAGS) $<

test: parser
	./parser examples.txt

clean:
	$(RM) -rf parser lex.yy.* *.o
