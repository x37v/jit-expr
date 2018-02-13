FILES = CMakeLists.txt Makefile configure src/
PROJECT_NAME = jit_expr
TARBALL = ${PROJECT_NAME}.tar.bz2

all: pd

clean:
	@echo "\033[33m< ---------------------- >\033[37m"
	@echo "\033[33m<     Cleaning Parser    >\033[37m"
	@echo "\033[33m< ---------------------- >\033[37m"
	@rm -f *.dot
	@if [ -e build/ ] ; then make -C build/ clean ; fi 1>/dev/null

distclean: clean
	@echo "\033[34m< Preparing directory for dist >\033[37m"
	@if [ -e build/ ] ; then rm -fr build/ ; fi 1>/dev/null
	@if test -e $(TARBALL) ; then rm -fr $(TARBALL) ; fi 1>/dev/null

dist: distclean
	@mkdir ${NAME}
	@cp -r $(FILES) ${NAME}
	@tar cjvf $(TARBALL) ${NAME}
	@rm -fr ${NAME}

pd:
	@make -C build

install:
	@make -C build install
