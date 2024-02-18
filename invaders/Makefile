NAME = "invaders"
COMPILER = clang++
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework OpenGL lib/libglfw3.a -framework Cocoa -framework IOKit -framework CoreVideo
OSX_BIN = -o "bin/$(NAME)"
C_FILES = src/*.cpp
CFLAGS += -I/opt/homebrew/Cellar/glew/2.2.0_1/include
LDFLAGS += -L/opt/homebrew/Cellar/glew/2.2.0_1/lib -lglew


game:
	$(COMPILER) $(C_FILES) $(SOURCE_LIBS) $(OSX_OPT) $(CFLAGS) $(LDFLAGS) $(OSX_BIN)

clean:
	rm bin/$(NAME)
