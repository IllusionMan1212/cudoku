BIN=cudoku
CC=gcc
CFLAGS=-Wall -Wextra -Werror -Wfloat-conversion -Wimplicit-fallthrough -pedantic -g `pkg-config --cflags freetype2` -I3rdparty/glad/include -I3rdparty/fmod/include
OBJ=main.o cudoku.o core.o shader.o text.o audio.o timer.o ui.o zephr.o zephr_math.o 3rdparty/glad/src/gl.o 3rdparty/glad/src/glx.o
LDFLAGS=`pkg-config --libs x11 freetype2` -lm -L3rdparty/fmod/lib -Wl,-rpath=3rdparty/fmod/lib -lfmod
DEPS=3rdparty/glad/include/glad/gl.h 3rdparty/glad/include/glad/glx.h 3rdparty/fmod/include/fmod.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm $(OBJ) $(BIN)
