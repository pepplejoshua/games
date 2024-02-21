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

struct Alien {
  size_t x, y;
  u8 type;
};

struct Player {
  size_t x, y;
  size_t life;
};

struct SpriteAnimation {
  bool loop;
  size_t num_frames;
  size_t frame_duration;
  size_t time;
  // a pointer to pointer to an array of sprites so
  // we can share frames
  Sprite **frames;
};

struct Bullet {
  size_t x, y;
  int dir;
};

#define GAME_MAX_BULLETS 128

struct Game {
  size_t width, height;
  size_t num_aliens;
  size_t num_bullets;
  Alien *aliens;
  Player player;
  Bullet bullets[GAME_MAX_BULLETS];
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

void key_callback(GLFWwindow *, int, int, int, int);

const size_t BUFFER_WIDTH = 224;
const size_t BUFFER_HEIGHT = 256;
const size_t BUFFER_SIZE = BUFFER_WIDTH * BUFFER_HEIGHT;
const size_t NUM_ALIENS = 55;
bool GAME_RUNNING = false;
int MOVE_DIR = 0;
bool FIRE_BUTTON_PRESSED = false;

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
  window = glfwCreateWindow(2 * BUFFER_WIDTH, 2 * BUFFER_HEIGHT,
                            "Space Invaders", NULL, NULL);
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

  // ..@.....@..
  // ...@...@...
  // ..@@@@@@@..
  // .@@.@@@.@@.
  // @@@@@@@@@@@
  // @.@@@@@@@.@
  // @.@.....@.@
  // ...@@.@@...
  Sprite alien_sprite;
  alien_sprite.width = 11;
  alien_sprite.height = 8;
  alien_sprite.data = new u8[88]{
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
      0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
      1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0};

  // ..@.....@..
  // @..@...@..@
  // @.@@@@@@@.@
  // @@@.@@@.@@@
  // @@@@@@@@@@@
  // .@@@@@@@@@.
  // ..@.....@..
  // .@.......@.
  Sprite alien_sprite_1;
  alien_sprite_1.width = 11;
  alien_sprite_1.height = 8;
  alien_sprite_1.data = new u8[88]{
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1,
      1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
      0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0};

  SpriteAnimation *alien_animation = new SpriteAnimation;
  alien_animation->loop = true;
  alien_animation->num_frames = 2;
  alien_animation->frame_duration = 10;
  alien_animation->time = 0;
  alien_animation->frames = new Sprite *[2];
  alien_animation->frames[0] = &alien_sprite;
  alien_animation->frames[1] = &alien_sprite_1;

  // .....@.....
  // ....@@@....
  // ....@@@....
  // .@@@@@@@@@.
  // @@@@@@@@@@@
  // @@@@@@@@@@@
  // @@@@@@@@@@@
  Sprite player_sprite = {
      .width = 11,
      .height = 7,
      .data =
          new u8[77]{
              0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
              0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
              1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
              1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          },
  };

  // @
  // @
  // @
  Sprite bullet_sprite = {.width = 1,
                          .height = 3,
                          .data = new u8[3]{
                              1,
                              1,
                              1,
                          }};

  Game game = {.width = BUFFER_WIDTH,
               .height = BUFFER_HEIGHT,
               .num_aliens = NUM_ALIENS,
               .num_bullets = 0,
               .aliens = new Alien[NUM_ALIENS],
               .player = {
                   .x = 112 - 5,
                   .y = 32,
                   .life = 3,
               }};

  // initialize the 55 aliens to reasonable positions
  for (size_t yi = 0; yi < 5; ++yi) {
    for (size_t xi = 0; xi < 11; ++xi) {
      game.aliens[yi * 11 + xi].x = 16 * xi + 20;
      game.aliens[yi * 11 + xi].y = 17 * yi + 128;
    }
  }

  // loop until the window should close
  u32 clear_color = rgb_to_u32(0, 128, 0);
  u32 red = rgb_to_u32(128, 0, 0);
  GAME_RUNNING = true;
  while (!glfwWindowShouldClose(window) && GAME_RUNNING) {
    buffer_clear(&buf, clear_color);

    // draw aliens
    for (size_t ai = 0; ai < game.num_aliens; ++ai) {
      const Alien &alien = game.aliens[ai];
      size_t current_frame =
          alien_animation->time / alien_animation->frame_duration;
      const Sprite &sprite = *alien_animation->frames[current_frame];
      buffer_draw_sprite(&buf, sprite, alien.x, alien.y, red);
    }

    // draw bullets
    for (size_t bi = 0; bi < game.num_bullets; ++bi) {
      const Bullet &bullet = game.bullets[bi];
      const Sprite &sprite = bullet_sprite;
      buffer_draw_sprite(&buf, sprite, bullet.x, bullet.y, red);
    }

    // draw player
    buffer_draw_sprite(&buf, player_sprite, game.player.x, game.player.y, red);

    // check animation of alien sprites and if it is still needed
    // or if it needs to restart the loop
    ++alien_animation->time;
    const size_t MAX_ANIMATION_TIME =
        alien_animation->num_frames * alien_animation->frame_duration;
    if (alien_animation->time == MAX_ANIMATION_TIME) {
      if (alien_animation->loop)
        alien_animation->time = 0;
      else {
        delete alien_animation;
        alien_animation = nullptr;
      }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buf.width, buf.height, GL_RGBA,
                    GL_UNSIGNED_INT_8_8_8_8, buf.data);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);

    // update the bullet positions using .dir and remove any projectiles out of
    // the game area
    for (size_t bi = 0; bi < game.num_bullets;) {
      game.bullets[bi].y += game.bullets[bi].dir;
      if (game.bullets[bi].y >= game.height ||
          game.bullets[bi].y < bullet_sprite.height) {
        game.bullets[bi] = game.bullets[game.num_bullets - 1];
        --game.num_bullets;
        continue;
      }

      ++bi;
    }

    // handle processing Player movement, if any
    int player_move_dir = 2 * MOVE_DIR;

    if (player_move_dir != 0) {
      if (game.player.x + player_sprite.width + player_move_dir >=
          game.width - 1) {
        game.player.x = game.width - player_sprite.width - player_move_dir - 1;
      } else if ((int)game.player.x + player_move_dir <= 0) {
        game.player.x = 0;
      } else {
        game.player.x += player_move_dir;
      }
    }

    if (FIRE_BUTTON_PRESSED && game.num_bullets < GAME_MAX_BULLETS) {
      game.bullets[game.num_bullets].x =
          game.player.x + player_sprite.width / 2;
      game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
      game.bullets[game.num_bullets].dir = 2;
      ++game.num_bullets;
    }
    FIRE_BUTTON_PRESSED = false;

    glfwPollEvents();
  }

  // clean up
  glfwDestroyWindow(window);
  glfwTerminate();

  glDeleteVertexArrays(1, &fullscreen_triangle_vao);

  delete[] alien_sprite.data;
  delete[] alien_sprite_1.data;
  delete[] alien_animation->frames;
  delete[] buf.data;
  delete[] game.aliens;

  delete alien_animation;

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
      FIRE_BUTTON_PRESSED = true;
    break;
  default:
    break;
  }
  printf("MOVE VALUE AFTER :: %d\n", MOVE_DIR);
}

/*
questions:
- what is a sprite?
- what is a texture?
- what is a vertex? how can I tell where a vertex is by looking at a sprite? who
computes where the vertex will be?
- what is a fragment? since the fragment shader comes after rasterization, what
is rasterization?
- seeing as we are using a quad i.e. triangle in our view port, how can I see a
square?
- what is in the sample2D buffer used in the fragment shader? Is that a
rasterized vertex?
- where does the sampled texture used by the fragment shader come from?
*/
