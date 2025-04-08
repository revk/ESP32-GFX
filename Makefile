7segpack: 7segpack.c
	cc -o 7segpack 7segpack.c -lpopt

fontpack: fontpack.c
	cc -o fontpack fontpack.c -lpopt

update:
	-git pull
	-git commit -a
	git submodule update --init --recursive --remote
	-git commit -a -m "Library update"
