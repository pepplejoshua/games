NAME = "snake"
COMPILER = clang
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL lib/libraylib.a
OSX_BIN = -o "bin/$(NAME)"
C_FILES = src/*.c

game:
	$(COMPILER) $(C_FILES) $(SOURCE_LIBS) $(OSX_OPT) $(OSX_BIN)

clean:
	rm bin/$(NAME)
