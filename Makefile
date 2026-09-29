.PHONY: all clean release

all:
	./tools/build.sh

clean:
	rm -rf build dist

release:
	./tools/release.sh
