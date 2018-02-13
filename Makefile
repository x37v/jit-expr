all: pd

clean:
	@rm -rf build

pd:
	@make -C build

install:
	@make -C build install
