# Compiles the main code                                               
compare: compare.c lists.c
	gcc compare.c lists.c -o compare -pthread -lm

# Compiles with debugging flag                                         
debug: compare.c lists.c
	gcc compare.c lists.c -o compare -pthread -lm -g

# Cleans the folder by removing .o files                               
clean:
	rm -f core compare *.o