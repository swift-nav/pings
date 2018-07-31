CC=gcc
CFLAGS+=-Wall -Wno-unused
SRC=lsn_edc.c lsn_fifo.c rtcm2sbp.c
INC=lsn_edc.h lsn_fifo.h lsn_common.h
INCDIR=-I./libsbp/c/include/ -I./gnss-converters/c/librtcm/c/include/ -I./gnss-converters/c/include/
LIBDIR=
LLIBS=-lm

SRC+=$(wildcard gnss-converters/c/src/*.c)
INC+=$(wildcard gnss-converters/c/include/*.c)

SRC+=$(wildcard gnss-converters/c/librtcm/c/src/*.c)
INC+=$(wildcard gnss-converters/c/librtcm/c/include/*.c)

all: rtcm2sbp

rtcm2sbp: $(SRC) $(INC) makefile
	#clang-format -i $(SRC)
	#clang-format -i $(INC)
	$(CC) $(CFLAGS) $(INCDIR) $(LIBDIR) $(SRC) -o $@ $(LLIBS)

clean:
	rm -fr ./rtcm2sbp *.o


