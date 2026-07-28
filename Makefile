TARGET=delaunay
SRCS=src/slot_array.cpp src/geometry.cpp src/graph.cpp src/main.cpp
OBJS=$(patsubst src/%.cpp,obj/%.o,$(SRCS))
LINK=-lraylib


$(TARGET): build $(OBJS)
	g++ $(OBJS) -o ./bin/$(TARGET) $(LINK)

$(OBJS): obj/%.o: src/%.cpp
	g++ -c $< -o $@ $(FLAGS)

debug:
	g++ $(SRCS) -g -o ./bin/debug $(LINK)

build:
	mkdir -p bin
	mkdir -p obj

clean:
	rm -vf ./bin/*

fclean: clean
	rm -vf ./obj/*

re: fclean $(TARGET)
