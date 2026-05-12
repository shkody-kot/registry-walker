CC = gcc
CFLAGS = -g -Wall -std=gnu99

SRC_DIRS = .

# Convert foo/bar.c -> build/foo/bar.o
OBJ = $(patsubst %.c,build/%.o,$(SRC))

all: registry-walk

registry-walk: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
	
# Pattern rule: build/foo/bar.o depends on foo/bar.c
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build registry-walk