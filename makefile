cc=g++
OBJ_DIR = ./obj
SRC_DIR = ./src
CFLAGS=-I./include
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, %.o, $(SRC))

ALL : select_test

select_test: $(OBJ)
	$(cc) $< -o $@
$(OBJ): $(SRC)
	$(cc) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJ) select_test

.PHONY: clean ALL
