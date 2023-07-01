INCLUDES = -I..

all:
	$(CC) $(INCLUDES) store_to_file.c -o store_to_file.out
	$(CC) $(INCLUDES) parse_and_print.c -o parse_and_print.out
	$(CC) $(INCLUDES) parse_string.c -o parse_string.out
	$(CC) $(INCLUDES) custom_io.cpp -o custom_io.out -lstdc++

clean:
	rm store_to_file.out parse_and_print.out parse_string.out custom_io.out