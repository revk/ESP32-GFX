fontpack: fontpack.c
	cc -o fontpack fontpack.c -lpopt

update:
	-git pull
	-git commit -a
	git submodule update --init --recursive --remote
	-git commit -a -m "Library update"
