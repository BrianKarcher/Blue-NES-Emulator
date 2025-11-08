TARGET=none
CONFIG=sprite.cfg
OBJECTFILE=./sprite.o
OUTFILE=./sprite.nes
DBGFILE=./sprite.dbg
MAPFILE=./sprite.map
LABELFILE=./sprite.lbl
LINKERFILE=./sprite.lnk
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