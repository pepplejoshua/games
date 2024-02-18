NAME = "invaders"
COMPILER = clang++
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework OpenGL lib/libglfw3.a -framework Cocoa -framework IOKit -framework CoreVideo
OSX_BIN = -o "bin/$(NAME)"
C_FILES = src/*.cpp

# Default target
game:
	$(COMPILER) $(C_FILES) $(SOURCE_LIBS) $(OSX_OPT) $(CFLAGS) $(LDFLAGS) $(OSX_BIN) -D$(DEVICE)

# Target for Mac Mini
mini: export CFLAGS = -I/opt/homebrew/Cellar/glew/2.2.0_1/include
mini: export LDFLAGS = -L/opt/homebrew/Cellar/glew/2.2.0_1/lib -lglew
mini: game


# Target for MacBook
book: export CFLAGS = -I/usr/local/Cellar/glew/2.2.0_1/
book: export LDFLAGS = -L/usr/local/Cellar/glew/2.2.0_1/lib -lglew
book: game

clean:
	rm -f bin/$(NAME)
