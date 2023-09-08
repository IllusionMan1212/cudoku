BIN=cudoku
CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -O3 -g `pkg-config --cflags x11` -I3rdparty/glad/include -I3rdparty/stb
OBJ=main.o x11.o cudoku.o shader.o 3rdparty/glad/src/gl.o 3rdparty/glad/src/glx.o 3rdparty/stb/stb.o
LDFLAGS=`pkg-config --libs x11` -lm
DEPS=3rdparty/glad/include/glad/gl.h 3rdparty/glad/include/glad/glx.h 3rdparty/stb/stb_image.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

