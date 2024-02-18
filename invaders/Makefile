__IWARILAMA_S_MAC_MINI__ = 0
__IWARILAMA_S_MACBOOK__ = 1

NAME = "invaders"
COMPILER = clang++
SOURCE_LIBS = -Ilib/
OSX_OPT = -Llib/ -framework OpenGL lib/libglfw3.a -framework Cocoa -framework IOKit -framework CoreVideo
OSX_BIN = -o "bin/$(NAME)"
C_FILES = src/*.cpp

ifeq ($(__IWARILAMA_S_MAC_MINI__), 1)
    CFLAGS = -I/opt/homebrew/Cellar/glew/2.2.0_1/include
    LDFLAGS = -L/opt/homebrew/Cellar/glew/2.2.0_1/lib -lglew
    DEVICE = "__IWARILAMA_S_MAC_MINI__ = 1"
endif

ifeq ($(__IWARILAMA_S_MACBOOK__), 1)
    CFLAGS = -I/usr/local/Cellar/glew/2.2.0_1/
    LDFLAGS = -L/usr/local/Cellar/glew/2.2.0_1/lib -lglew
    DEVICE = "__IWARILAMA_S_MACBOOK__ = 1"
endif

game:
	$(COMPILER) $(C_FILES) $(SOURCE_LIBS) $(OSX_OPT) $(CFLAGS) $(LDFLAGS) $(OSX_BIN) -D$(DEVICE)

clean:
	rm bin/$(NAME)
