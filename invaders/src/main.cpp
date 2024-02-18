#include "/opt/homebrew/Cellar/glew/2.2.0_1/include/GL/glew.h"

#include "../lib/glfw3.h"
#include <cstdio>

void error_callback(int error, const char *desc) {
  fprintf(stderr, "Error: %s\n", desc);
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
