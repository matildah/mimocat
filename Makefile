all: recv send 

recv: recv.c net.c pack.c
	cc -o recv recv.c net.c pack.c

send: send.c net.c pack.c
	cc -o send send.c net.c pack.c

clean:
	rm -f send recv
