TARGET=delaunay
SRCS=lib/predicates.c src/slot_array.cpp src/geometry.cpp src/graph.cpp src/bowyer_watson.cpp src/main.cpp
OBJS_TMP=$(SRCS:.cpp=.o)
OBJS=$(addprefix obj/, $(notdir $(OBJS_TMP:.c=.o)))
CXXFLAGS=-O3 -Wall -I./include -I./lib
CFLAGS=-O3 -Wall -I./include -I./lib
LINK=-lraylib
VPATH=src:lib

$(TARGET): build $(OBJS)
	g++ $(OBJS) -o ./bin/$(TARGET) $(LINK)

obj/%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

obj/%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

debug:
	g++ $(CXXFLAGS) -g $(SRCS) -o ./bin/debug $(LINK)

build:
	mkdir -p bin obj

clean:
	rm -vf ./bin/*

fclean: clean
	rm -vf ./obj/*

re: fclean $(TARGET)
