CC=cc
LINKER=-ldl -lm -lfcgi -lmagic
FLAGS=-Wall -g3 -march=native
TARGET=./filescgi
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
DEP=$(OBJ:.o=.d) # one dependency file for each source

ADDR=127.0.0.1
PORT=5000

all: $(TARGET)

%.d: %.c
	@$(CC) $(FLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(FLAGS) $(PREPROCESSPARMS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJ) $(LINKER)

clean: clean-obj clean-bin

clean-obj:
	rm -f $(OBJ) $(DEP)
	
clean-bin:
	rm -f $(shell find . -maxdepth 1 -executable -type f)

start:
	cgi-fcgi -connect $(ADDR):$(PORT) $(TARGET)

kill:
	ps | grep "prog" | cut -d " " -f 1 | xargs -n 1 kill 

