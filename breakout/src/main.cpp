#include "../lib/glew.h"

#include "../lib/glfw3.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

typedef uint32_t u32;
typedef uint8_t u8;
typedef size_t usize;

void error_callback(int error, const char *desc) {
  fprintf(stderr, "Error: %s\n", desc);
}

// we will store the pixels in u32 sized integers but we will only use
// the leftmost 24 bits (8 bits for r-g-b)
struct Buffer {
  usize width, height;
  u32 *data;
};

// this struct will hold a bitmapped sprite which will be drawn to screen
// with a specified color. each pixel is 8 bits wide
struct Sprite {
  usize width, height;
  u8 *data;
};

struct Player {
  usize x, y;
  usize turns = 3;
};

struct Ball {
  usize x, y;
  struct {
    usize x, y;
  } vel;
};

enum BrickScore : u8 {
  YELLOW = 1,
  GREEN = 3,
  ORANGE = 5,
  RED = 7,
};

struct Brick {
  usize x, y;
  // yellow: 1 point
  // green: 3 points
  // orange: 5 points
  // red: 7 points
  BrickScore value;
  u32 color;
};

struct Game {
  usize num_bricks;
  Brick *bricks;
  Player player;
  Ball ball;
  usize score = 0;
};

u32 rgb_to_u32(u8 r, u8 g, u8 b) {
  return (r << 24) | (g << 16) | (b << 8) | 255;
}

void buffer_clear(Buffer *buf, u32 color) {
  for (usize i = 0; i < buf->width * buf->height; ++i) {
    buf->data[i] = color;
  }
}

void buffer_draw_sprite(Buffer *buf, const Sprite &sprite, usize x, usize y,
                        u32 color) {
  for (usize xi = 0; xi < sprite.width; ++xi) {
    for (usize yi = 0; yi < sprite.height; ++yi) {
      usize sy = sprite.height - 1 + y - yi;
      usize sx = x + xi;
      if (sprite.data[yi * sprite.width + xi] && sy < buf->height &&
          sx < buf->width) {
        buf->data[sy * buf->width + sx] = color;
      }
    }
  }
}

// this draws text from the spritesheet (which contains space, upper case
// alphabets, numbers and some special characters).
void buffer_draw_text(Buffer *buf, const Sprite &text_spritesheet,
                      const char *text, usize x, usize y, u32 color) {
  usize xp = x;
  // which is 5 * 7
  usize stride = text_spritesheet.width * text_spritesheet.height;
  Sprite sprite = text_spritesheet;
  for (const char *charp = text; *charp != '\0'; ++charp) {
    // get the index for the character in our array
    // since space is 32, it will be the 0th element in the spritesheet
    char chari = *charp - 32;

    if (chari < 0 || chari >= 65)
      continue;

    // move the pointer to the start of the sprite
    sprite.data = text_spritesheet.data + (chari * stride);
    buffer_draw_sprite(buf, sprite, xp, y, color);
    // move the x coordinate to account for the printed character and extra
    // space
    xp += sprite.width + 1;
  }
}

void buffer_draw_number(Buffer *buf, const Sprite &number_spritesheet,
                        usize number, usize x, usize y, uint32_t color) {
  u8 digits[64];
  usize num_digits = 0;
  usize cur_num = number;
  do {
    digits[num_digits++] = cur_num % 10;
    cur_num /= 10;
  } while (cur_num > 0);

  usize xp = x;
  usize stride = number_spritesheet.height * number_spritesheet.width;
  Sprite sprite = number_spritesheet;
  for (int i = num_digits - 1; i >= 0; --i) {
    u8 digit = digits[i];
    sprite.data = number_spritesheet.data + (digit * stride);
    buffer_draw_sprite(buf, sprite, xp, y, color);
    xp += sprite.width + 1;
  }
}

bool sprite_overlap_check(const Sprite &Sp_a, usize x_a, usize y_a,
                          const Sprite &Sp_b, usize x_b, usize y_b) {
  // for 2 sprites to be collide, all these have to be true
  // = B's top right edge > A's x: x_a < x_b + Sp_b.width
  // = A's top right edge > B's x: x_a + Sp_a.width > x_b
  // = B's bottom left edge > A's y: y_a < y_b + Sp_b.height
  // = A's bottom left edge > B's y: y_a + Sp_a.height > y_b
  if (x_a < x_b + Sp_b.width && x_a + Sp_a.width > x_b &&
      y_a < y_b + Sp_b.height && y_a + Sp_a.height > y_b) {
    return true;
  }
  return false;
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

void key_callback(GLFWwindow *, int, int, int, int);

const usize BUFFER_WIDTH = 226;
const usize BUFFER_HEIGHT = 200;
const usize BUFFER_SIZE = BUFFER_WIDTH * BUFFER_HEIGHT;
const usize ROW_OF_BRICKS = 8;  // 8
const usize COL_OF_BRICKS = 14; // 14
const usize NUM_BRICKS = ROW_OF_BRICKS * COL_OF_BRICKS;

bool GAME_RUNNING = false;
int MOVE_DIR = 0;
bool GAME_IS_ON = false;

int main() {
  glfwSetErrorCallback(error_callback);

  GLFWwindow *window;

  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // make a window and set the error callback
  window = glfwCreateWindow(2 * BUFFER_WIDTH, 2 * BUFFER_HEIGHT, "breakout",
                            NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  // set callback to handle user input
  glfwSetKeyCallback(window, key_callback);

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

  glfwSwapInterval(1);

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

  // make generic sprite for the different blocks
  // @@@@@@@@@@@@@@
  // @@@@@@@@@@@@@@
  // @@@@@@@@@@@@@@
  // @@@@@@@@@@@@@@
  Sprite brick;
  brick.width = 14;
  brick.height = 4;
  brick.data = new u8[56]{
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };

  // @@@
  // @@@
  Sprite ball_s;
  ball_s.width = 3;
  ball_s.height = 3;
  ball_s.data = new u8[9]{
      1, 1, 1, 1, 1, 1, 1, 1, 1,
  };

  // @@@@@@@@@@@@@@
  // @@@@@@@@@@@@@@
  // @@@@@@@@@@@@@@
  Sprite player_s;
  player_s.width = 14;
  player_s.height = 3;
  player_s.data = new u8[42]{
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };

  u32 black_clear_color = rgb_to_u32(0, 0, 0);
  u32 red = rgb_to_u32(200, 0, 0);
  u32 green = rgb_to_u32(0, 128, 0);
  u32 yellow = rgb_to_u32(255, 255, 0);
  u32 orange = rgb_to_u32(255, 165, 0);

  GAME_RUNNING = true;
  int player_move_dir = 0;
  usize score = 0;

  Game game = {
      .num_bricks = NUM_BRICKS,
      .bricks = new Brick[NUM_BRICKS],
      .player =
          {
              .x = BUFFER_WIDTH / 2 - (player_s.width / 2),
              .y = 25 - player_s.height,
          },
      .ball =
          {
              .x = BUFFER_WIDTH / 2 - (ball_s.width / 2),
              .y = game.player.y + ball_s.height,
              .vel =
                  {// it will move straight up since the y axis is inverted
                   .x = 0,
                   .y = 1},
          },
  };

  // starting brick arrangement: 16 x 8
  // initialize the bricks to this arrangement
  // Red: Row 1
  // Orange: Row 2
  // Green: Row 3
  // Yellow: Row 4
  for (usize yi = 0; yi < ROW_OF_BRICKS; ++yi) {
    for (usize xi = 0; xi < COL_OF_BRICKS; ++xi) {
      Brick &bricki = game.bricks[yi * COL_OF_BRICKS + xi];
      // 16 ensures 16 pixels between each brick
      // 5 ensures 5 pixels from the left edge of the screen
      // 8 ensures 8 pixels between each brick horizontally
      // bricki.x = 16 * xi + 5 + (4 * xi);
      bricki.x = 16 * xi +
                 2; // Corrected calculation for consistent horizontal spacing
      // 10 ensures 10 pixels between each brick vertically
      bricki.y = 8 * yi + 130;

      if (yi == 0 || yi == 1) {
        bricki.color = red;
      } else if (yi == 2 || yi == 3) {
        bricki.color = orange;
      } else if (yi == 4 || yi == 5) {
        bricki.color = green;
      } else if (yi == 6 || yi == 7) {
        bricki.color = yellow;
      }
    }
  }

  while (!glfwWindowShouldClose(window) && GAME_RUNNING) {
    buffer_clear(&buf, black_clear_color);

    // draw some blocks to test
    for (usize i = 0; i < game.num_bricks; ++i) {
      buffer_draw_sprite(&buf, brick, game.bricks[i].x, game.bricks[i].y,
                         game.bricks[i].color);
    }

    // draw ball sprite
    buffer_draw_sprite(&buf, ball_s, game.ball.x, game.ball.y, orange);

    // draw player sprite
    buffer_draw_sprite(&buf, player_s, game.player.x, game.player.y, green);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf.width, buf.height, GL_RGBA,
                    GL_UNSIGNED_INT_8_8_8_8, buf.data);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  // clean up
  glfwDestroyWindow(window);
  glfwTerminate();

  glDeleteVertexArrays(1, &fullscreen_triangle_vao);

  delete[] game.bricks;
  delete[] brick.data;
  delete[] ball_s.data;
  delete[] player_s.data;

  return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  // const char *action_s =
  //     action == GLFW_PRESS
  //         ? "<pressed>"
  //         : (action == GLFW_RELEASE ? "<released>" : "<other>");
  // printf("MOVE VALUE BEFORE :: %d (%s)\n", MOVE_DIR, action_s);
  switch (key) {
  case GLFW_KEY_ESCAPE:
    if (action == GLFW_PRESS)
      GAME_RUNNING = false;
    break;
  case GLFW_KEY_RIGHT:
    if (action == GLFW_PRESS)
      MOVE_DIR += 1;
    else if (action == GLFW_RELEASE)
      MOVE_DIR -= 1;
    break;
  case GLFW_KEY_LEFT:
    if (action == GLFW_PRESS)
      MOVE_DIR -= 1;
    else if (action == GLFW_RELEASE)
      MOVE_DIR += 1;
    break;
  case GLFW_KEY_SPACE:
    if (action == GLFW_RELEASE)
      GAME_IS_ON = true;
    break;
  default:
    break;
  }
  // printf("MOVE VALUE AFTER :: %d\n", MOVE_DIR);
}
