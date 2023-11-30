all: pword tword

tword: tword.c
	gcc -Wall -g -o tword tword.c -lpthread 

pword: pword.c
	gcc -Wall -g -o pword pword.c -lrt

clean:
	rm -fr tword pword *~ output*