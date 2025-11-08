TARGET=none
CONFIG=hello_world.cfg
OBJECTFILE=./hello_world.o
OUTFILE=./hello_world.nes
DBGFILE=./hello_world.dbg
MAPFILE=./hello_world.map
LABELFILE=./hello_world.lbl
LINKERFILE=./hello_world.lnk
SOURCE=init.asm

.PHONY: build clean run env-emulator-path

build:
	ca65 -g -l ${LINKERFILE} -t ${TARGET} ${SOURCE} -o ${OBJECTFILE}
	ld65 -Ln ${LABELFILE} -m ${MAPFILE} -vm --dbgfile ${DBGFILE} -o ${OUTFILE} -C ${CONFIG} ${OBJECTFILE} ${TARGET}.lib

run: env-emulator-path
	${EMULATOR_PATH} ${OUTFILE}

env-emulator-path:
ifndef EMULATOR_PATH
	$(error EMULATOR_PATH is not set)
endif