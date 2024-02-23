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

struct Game {};

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

// this draws text from the spritesheet (which contains space, upper case
// alphabets, numbers and some special characters).
void buffer_draw_text(Buffer *buf, const Sprite &text_spritesheet,
                      const char *text, size_t x, size_t y, u32 color) {
  size_t xp = x;
  // which is 5 * 7
  size_t stride = text_spritesheet.width * text_spritesheet.height;
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
                        size_t number, size_t x, size_t y, uint32_t color) {
  u8 digits[64];
  size_t num_digits = 0;
  size_t cur_num = number;
  do {
    digits[num_digits++] = cur_num % 10;
    cur_num /= 10;
  } while (cur_num > 0);

  size_t xp = x;
  size_t stride = number_spritesheet.height * number_spritesheet.width;
  Sprite sprite = number_spritesheet;
  for (int i = num_digits - 1; i >= 0; --i) {
    u8 digit = digits[i];
    sprite.data = number_spritesheet.data + (digit * stride);
    buffer_draw_sprite(buf, sprite, xp, y, color);
    xp += sprite.width + 1;
  }
}

bool sprite_overlap_check(const Sprite &Sp_a, size_t x_a, size_t y_a,
                          const Sprite &Sp_b, size_t x_b, size_t y_b) {
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

const size_t BUFFER_WIDTH = 224;
const size_t BUFFER_HEIGHT = 256;
const size_t BUFFER_SIZE = BUFFER_WIDTH * BUFFER_HEIGHT;

bool GAME_RUNNING = false;
int MOVE_DIR = 0;

int main() { return 0; }
