SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=%.o)
FLAGS := -ggdb

%.o: src/%.cpp
	c++ $< $(FLAGS) -std=c++23 -c -o $@

inn: $(OBJ)
	c++ $^ -fsanitize=address,undefined -o $@

clean:
	rm $(OBJ) ./inn
