#include "../lib/glew.h"

#include "../lib/glfw3.h"
#include <cstddef>
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

// this struct will hold a bitmapped sprite which will be drawn to screen
// with a specified color. each pixel is 8 bits wide
struct Sprite {
  size_t width, height;
  u8 *data;
};

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer *buf, u32 color) {
  for (size_t i = 0; i < buf->width * buf->height; ++i) {
    buf->data[i] = color;
  }
}

void buffer_draw_sprite(Buffer *buf, const Sprite &sprite, size_t x, size_t y,
                        u32 color) {
  for (size_t xi = 0; xi < sprite.width; ++xi) {
    for (size_t yi = 0; yi < sprite.height; ++yi) {
      size_t sy = sprite.height - 1 + y - yi;
      size_t sx = x + xi;
      if (sprite.data[yi * sprite.width + xi] && sy < buf->height &&
          sx < buf->width) {
        buf->data[sy * buf->width + sx] = color;
      }
    }
  }
}

void validate_shader(GLuint shader, const char *file = 0) {
  static const unsigned int BUFFER_SIZE = 512;
  char buffer[BUFFER_SIZE];
  GLsizei length = 0;

  glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

  if (length > 0) {
    printf("Shader %d(%s) compile error: %s\n", shader, (file ? file : ""),
           buffer);
  }
}

bool validate_program(GLuint program) {
  static const unsigned int BUFFER_SIZE = 512;
  char buffer[BUFFER_SIZE];
  GLsizei length = 0;

  glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

  if (length > 0) {
    printf("Program %d link error: %s\n", program, buffer);
    return false;
  }

  return true;
}

const size_t BUFFER_WIDTH = 224;
const size_t BUFFER_HEIGHT = 256;
const size_t BUFFER_SIZE = BUFFER_WIDTH * BUFFER_HEIGHT;

int main() {
  glfwSetErrorCallback(error_callback);

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

  glClearColor(1.0, 0.0, 0.0, 1.0);

  // create and clear buffer
  Buffer buf = {.width = BUFFER_WIDTH,
                .height = BUFFER_HEIGHT,
                .data = new u32[BUFFER_SIZE]};
  buffer_clear(&buf, 0);

  // img data is transferred to the gpu using textures
  // a texture (in terms of vao) is an object holding img data,
  // and other info about formatting of the data

  // first we create the buffer to hold the texture
  GLuint buf_texture;
  glGenTextures(1, &buf_texture);

  // then we specify img format and std parameters about the
  // behavior of the sampling of the texture
  glBindTexture(GL_TEXTURE_2D, buf_texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buf.width, buf.height, 0, GL_RGBA,
               GL_UNSIGNED_INT_8_8_8_8, buf.data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // create VAO to track all appropriate vertex data for rendering
  GLuint fullscreen_triangle_vao;
  glGenVertexArrays(1, &fullscreen_triangle_vao);

  static const char *vertex_shader =
      "\n"
      "#version 330\n"
      "\n"
      "noperspective out vec2 TexCoord;\n"
      "\n"
      "void main(void) {\n"
      "\n"
      "    TexCoord.x = (gl_VertexID == 2) ? 2.0 : 0.0;\n"
      "    TexCoord.y = (gl_VertexID == 1) ? 2.0 : 0.0;\n"
      "    \n"
      "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
      "}\n";

  static const char *fragment_shader =
      "\n"
      "#version 330\n"
      "\n"
      "uniform sampler2D buffer;\n"
      "noperspective in vec2 TexCoord;\n"
      "\n"
      "out vec3 outColor;\n"
      "\n"
      "void main(void) {\n"
      "\n"
      "    outColor = texture(buffer, TexCoord).rgb;\n"
      "}\n";

  // compile shader programs to gpu friendly format
  GLuint shader_id = glCreateProgram();

  // compile vertex shader
  {
    GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(shader_vp, 1, &vertex_shader, 0);
    glCompileShader(shader_vp);
    validate_shader(shader_vp, vertex_shader);
    glAttachShader(shader_id, shader_vp);

    glDeleteShader(shader_vp);
  }

  // compile fragment shader
  {
    GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(shader_fp, 1, &fragment_shader, 0);
    glCompileShader(shader_fp);
    validate_shader(shader_fp, fragment_shader);
    glAttachShader(shader_id, shader_fp);

    glDeleteShader(shader_fp);
  }

  glLinkProgram(shader_id);

  if (!validate_program(shader_id)) {
    fprintf(stderr, "Error while validating shader.\n");
    glfwTerminate();
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);
    delete[] buf.data;
    return -1;
  }

  glUseProgram(shader_id);

  // attach the texture to the sampler2D buffer variable in the fragment shader
  GLint frag_buf_location = glGetUniformLocation(shader_id, "buffer");
  glUniform1i(frag_buf_location, 0);

  // open gl settings
  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);

  glBindVertexArray(fullscreen_triangle_vao);

  // make alien sprite
  Sprite alien_sprite;
  alien_sprite.width = 11;
  alien_sprite.height = 8;
  // ..@.....@..
  // ...@...@...
  // ..@@@@@@@..
  // .@@.@@@.@@.
  // @@@@@@@@@@@
  // @.@@@@@@@.@
  // @.@.....@.@
  // ...@@.@@...
  alien_sprite.data = new u8[88]{
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
      1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0};

  // loop until the window should close
  u32 clear_color = rgb_to_u32(0, 128, 0);

  while (!glfwWindowShouldClose(window)) {
    // glClear(GL_COLOR_BUFFER_BIT);
    buffer_clear(&buf, clear_color);
    buffer_draw_sprite(&buf, alien_sprite, 112, 128, rgb_to_u32(128, 0, 0));

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf.width, buf.height, GL_RGBA,
                    GL_UNSIGNED_INT_8_8_8_8, buf.data);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up
  glfwDestroyWindow(window);
  glfwTerminate();

  glDeleteVertexArrays(1, &fullscreen_triangle_vao);

  delete[] buf.data;
  delete[] alien_sprite.data;

  return 0;
}
