INCLUDES = -I..

all:
	$(CC) $(INCLUDES) example.c -o example.out
	$(CC) $(INCLUDES) read_from_file.cpp -lstdc++ -o read_from_file_cpp.out
	$(CC) $(INCLUDES) from_string.c -o from_string.out

clean:
	rm example.out