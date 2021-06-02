CC=cc
LINKER=-ldl -lpthread -lm -lmagic
CFLAGS=-fPIC -Wall -g3 -march=native
TARGET=./paste
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
DEP=$(OBJ:.o=.d) # one dependency file for each source

ADDR=127.0.0.1
PORT=5000

all: $(TARGET) ext_uuid.so

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

ext_uuid.so: src/sqlite3.o src/uuid.o
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LINKER)

$(TARGET): src/sqlite3.o src/paste.o
	$(CC) $(CFLAGS) -o $(TARGET) $^ $(LINKER)

clean: clean-obj clean-bin

clean-obj:
	rm -f $(OBJ) $(DEP)
	
clean-bin:
	rm -f $(TARGET) ext_uuid.so

