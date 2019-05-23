all: write read clear undo free_space tests

tests: tests.o cdm_utils.o
	gcc -o tests tests.o cdm_utils.o

free_space: free_space.o cdm_utils.o
	gcc -o free_space free_space.o cdm_utils.o

undo: undo.o cdm_utils.o
	gcc -o undo undo.o cdm_utils.o

clear: clear.o cdm_utils.o
	gcc -o clear clear.o cdm_utils.o

read: read.o cdm_utils.o
	gcc -o read read.o cdm_utils.o

write: write.o cdm_utils.o
	gcc -o write write.o cdm_utils.o

tests.o: tests.c
	gcc -c tests.c

free_space.o: free_space.c
	gcc -c free_space.c

undo.o: undo.c
	gcc -c undo.c

clear.o: clear.c
	gcc -c clear.c

read.o: read.c
	gcc -c read.c

write.o: write.c
	gcc -c write.c

cdm_utils.o: cdm_utils.c
	gcc -c cdm_utils.c

clean:
	rm -f *.o write read clear undo free_space tests