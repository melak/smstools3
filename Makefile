# Makefile


compile: 
	cd src && $(MAKE) -$(MAKEFLAGS) 

install: compile
	./install.sh

uninstall: 
	./uninstall.sh

clean:
	cd src && $(MAKE) -$(MAKEFLAGS) clean

package: compile clean
	./package.sh
