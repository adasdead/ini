INCLUDES = -I..
CC ?= cc

ifeq ($(OS),Windows_NT)
	RM = del /Q
	EXE = .exe
else
	RM = rm -f
	EXE = .bin
endif

all:
	$(CC) $(INCLUDES) store_to_file.c -o store_to_file$(EXE)
	$(CC) $(INCLUDES) parse_and_print.c -o parse_and_print$(EXE)
	$(CC) $(INCLUDES) parse_string.c -o parse_string$(EXE)
	$(CC) $(INCLUDES) custom_io.cpp -o custom_io$(EXE) -lstdc++

clean:
	$(RM) store_to_file$(EXE)
	$(RM) parse_and_print$(EXE)
	$(RM) parse_string$(EXE)
	$(RM) custom_io$(EXE)