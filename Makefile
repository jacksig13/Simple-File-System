all:
	gcc -g fs.c filefs.c -o filefs

clean:
	rm filefs
