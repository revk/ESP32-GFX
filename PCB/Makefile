stl:	EPD/EPD.stl EPDBat/EPDBat.stl

PCBCase/case: PCBCase/case.c
	make -C PCBCase

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@
	echo "Made $@"

EPD/EPD.scad: EPD/EPD.kicad_pcb PCBCase/case Makefile PCBCase/case.scad
	PCBCase/case -o $@ $< --top=6 --base=1 --pcb=2 --case=2

EPDBat/EPDBat.scad: EPDBat/EPDBat.kicad_pcb PCBCase/case Makefile PCBCase/case.scad
	PCBCase/case -o $@ $< --top=6 --base=1 --pcb=2 --case=2

