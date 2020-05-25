all: duct

duct: source.c
	gcc -O2 -o duct source.c 

clean:
	rm -f duct
