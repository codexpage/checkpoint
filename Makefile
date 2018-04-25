CFLAGS=-g -O0

all:	check

default: check

clean-ckpt:
	rm -rf myckpt

clean: clean-ckpt
	rm -rf myrestart hello libckpt.so

check: clean lib hello
	(sleep 3 && kill -12 `pgrep -n hello` && sleep 2 && pkill -9 -n hello && make restart) &
	LD_PRELOAD=./libckpt.so ./hello

lib:
	gcc -g -O0 -fpic -shared \
	 -fno-stack-check -fstack-protector-explicit -fno-stack-protector \
	sig.c -o libckpt.so

hello:
	gcc -g -O0 hello.c -fpic \
	-fno-stack-check -fstack-protector-explicit -fno-stack-protector \
	-o hello

restart: hello lib
	gcc -g -O0 -static -fpic \
	-Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	-fno-stack-check -fstack-protector-explicit -fno-stack-protector \
	 myrestart.c  -o myrestart
	./myrestart myckpt


gdb:
	gdb  --args ./myrestart myckpt
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
