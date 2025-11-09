TARGET=none
CONFIG=vert_scroll.cfg
OBJECTFILE=./vert_scroll.o
OUTFILE=./vert_scroll.nes
SHORTOUTFILE=vert_scroll.nes
DBGFILE=./vert_scroll.dbg
MAPFILE=./vert_scroll.map
LABELFILE=./vert_scroll.lbl
LINKERFILE=./vert_scroll.lnk
SOURCE=init.asm

# Cross-platform mkdir/copy helpers. On Windows make sets OS=Windows_NT.
ifeq ($(OS),Windows_NT)
MKDIR_CMD=if not exist ..\\..\\roms mkdir ..\\..\\roms
COPY_CMD=copy /Y ${SHORTOUTFILE} ..\\..\\roms\\
else
MKDIR_CMD=mkdir -p ../../roms
COPY_CMD=cp ${OUTFILE} ../../roms/
endif

.PHONY: build clean run env-emulator-path

build:
	ca65 -g -l ${LINKERFILE} -t ${TARGET} ${SOURCE} -o ${OBJECTFILE}
	ld65 -Ln ${LABELFILE} -m ${MAPFILE} -vm --dbgfile ${DBGFILE} -o ${OUTFILE} -C ${CONFIG} ${OBJECTFILE} ${TARGET}.lib
	@echo "Ensuring output directory exists..."
	@$(MKDIR_CMD)
	@echo "Copying ${OUTFILE} to ../../roms/"
	@$(COPY_CMD)
run: env-emulator-path
	${EMULATOR_PATH} ${OUTFILE}

env-emulator-path:
ifndef EMULATOR_PATH
	$(error EMULATOR_PATH is not set)
endif