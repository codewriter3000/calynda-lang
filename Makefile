.PHONY: all test clean install compiler mcp-server

all: compiler

compiler:
	$(MAKE) -C compiler all

test:
	$(MAKE) -C compiler test

clean:
	$(MAKE) -C compiler clean

install:
	cd compiler && sh ./install.sh

mcp-server:
	npm --prefix mcp-server run build
