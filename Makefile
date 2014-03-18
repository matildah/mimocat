all: recv send 

recv: recv.c net.c pack.c
	clang -Wall -Weverything -o recv recv.c net.c pack.c

send: send.c net.c pack.c
	clang -Wall -Weverything -o send send.c net.c pack.c

clean:
	rm -f send recv
