all: example_file_c example_file_cpp example_str_c

example_file_c: example_file.c
	gcc -o example_file_c example_file.c -I../

example_file_cpp: example_file.cpp
	gcc -o example_file_cpp example_file.cpp -I../ -lstdc++

example_str_c: example_str.c
	gcc -o example_str_c example_str.c -I../