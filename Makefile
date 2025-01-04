CC = g++ -std=c++17
CFLAGS = -Wall -Wextra
LIBS = -lsdl2 -lsdl2_ttf
PROGRAM = a.out
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

$(PROGRAM): $(OBJECTS)
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(PROGRAM)
