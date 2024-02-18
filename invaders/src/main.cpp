#include "../lib/glew.h"

#include "../lib/glfw3.h"
#include <cstdint>
#include <cstdio>

typedef uint32_t u32;
typedef uint8_t u8;

void error_callback(int error, const char *desc) {
  fprintf(stderr, "Error: %s\n", desc);
}

// we will store the pixels in u32 sized integers but we will only use
// the leftmost 24 bits (8 bits for r-g-b)
struct Buffer {
  size_t width, height;
  u32 *data;
};

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer *buf, u32 color) {
  for (size_t i = 0; i < buf->width * buf->height; ++i) {
    buf->data[i] = color;
  }
}

int main() {
  GLFWwindow *window;

  // try to init the library
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // make a window and set the error callback
  window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwSetErrorCallback(error_callback);

  // make the window the current context
  glfwMakeContextCurrent(window);

  // init GLEW
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    fprintf(stderr, "Error initializing GLEW.\n");
    glfwTerminate();
    return -1;
  }

  // get open GL versions
  int glVersion[2] = {-1, 1};
  glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
  glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
  printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);

  // loop until the window should close
  glClearColor(1.0, 0.0, 0.0, 1.0);
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up
  glfwTerminate();
  return 0;
}
