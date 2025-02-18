#include <iostream>
#include <chrono>
#include <thread>
#include <sixel.h>

#include <embedder.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

// terminal mouse
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h> // Added this header for ioctl
#include <termios.h>
#include <unistd.h>
// terminal mouse

// This value is calculated after the window is created.
static double g_pixelRatio = 2.0f;
// Initial window width
static const size_t kInitialWindowWidth = 1280;
// Initial window height
static const size_t kInitialWindowHeight = 800;
// Flutter View Id
static constexpr FlutterViewId kImplicitViewId = 0;
#define ESC '\033'

int frameNo = 0;

FlutterEngine engine = nullptr;

// bool SaveToPng(void *user_data,
//                      const void *buffer,
//                      size_t row_bytes,
//                      size_t height)
// {
//   if (!buffer || !buffer)
//   {
//     fprintf(stderr, "Invalid buffer in PresentSurface.\n");
//     return false;
//   }

//   // Save the buffer to a BMP file
//   const char *output_filename = "output.bmp";
//   bool success = SaveAsBMP(output_filename, buffer, kInitialWindowWidth, height, row_bytes);

//   if (success)
//   {
//     printf("Rendered frame saved as BMP: %s\n", output_filename);
//   }
//   else
//   {
//     fprintf(stderr, "Failed to save BMP file.\n");
//   }

//   return success;
// }

// void print_colored_text(int r, int g, int b, const char *text)
// {
//   // ANSI Escape Code for True Color (24-bit)
//   printf("\033[38;2;%d;%d;%dm%s\033[0m", r, g, b, text);
// }

// void print_pixels(const void *allocation, size_t width, size_t height)
// {
//   const uint8_t *pixels = (const uint8_t *)allocation; // Cast to byte pointer

//   for (size_t y = 0; y < height; y++)
//   {
//     for (size_t x = 0; x < width; x++)
//     {
//       size_t index = (y * width + x) * 4; // Each pixel has 4 bytes (RGBA)
//       uint8_t r = pixels[index];
//       uint8_t g = pixels[index + 1];
//       uint8_t b = pixels[index + 2];
//       uint8_t a = pixels[index + 3];

//       // printf("Pixel (%zu, %zu): R=%d, G=%d, B=%d, A=%d\n", x, y, r, g, b, a);
//       // printf("(R=%d, G=%d, B=%d, A=%d); ", r, g, b, a);
//       print_colored_text(r, g, b, "█");
//     }
//     printf("\n");
//   }
// }

static int sixel_write(char *data, int size, void *priv)
{
  return fwrite(data, 1, size, (FILE *)priv);
}

void GetPixelComponents(uint32_t argb, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
  *a = (argb >> 24) & 0xFF;
  *r = (argb >> 16) & 0xFF;
  *g = (argb >> 8) & 0xFF;
  *b = argb & 0xFF;
}

void print_pixels_sixel(const void *allocation, size_t width, size_t height)
{
  const uint8_t *local_pixels = (const uint8_t *)allocation; // Cast to byte pointer

  sixel_output_t *output;
  sixel_dither_t *dither;

  sixel_output_new(&output, sixel_write, stdout, NULL);
  dither = sixel_dither_get(SIXEL_BUILTIN_XTERM256);
  sixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_ARGB8888);

  // good first frame
  unsigned char *pixels;
  pixels = (unsigned char *)malloc(kInitialWindowWidth * kInitialWindowHeight * 4);

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      size_t index = (y * width + x) * 4;
      uint8_t r = local_pixels[index];
      uint8_t g = local_pixels[index + 1];
      uint8_t b = local_pixels[index + 2];
      uint8_t a = local_pixels[index + 3];

      pixels[(y * width + x) * 4 + 0] = a;
      pixels[(y * width + x) * 4 + 1] = r;
      pixels[(y * width + x) * 4 + 2] = g;
      pixels[(y * width + x) * 4 + 3] = b;
    }
  }
  printf("\033[H");

  sixel_encode(pixels, width, height, 0, dither, output);
  fflush(stdout);
  free(pixels);

  sixel_dither_unref(dither);
  sixel_output_unref(output);
}

bool PresentSurface(void *user_data,
                    const void *allocation,
                    size_t row_bytes,
                    size_t height)
{
  const uint32_t *pixels = static_cast<const uint32_t *>(allocation);
  size_t width = row_bytes / 4; // 4 bytes per pixel in ARGB

  size_t x = 100; // example x coordinate
  size_t y = 100;

  print_pixels_sixel(allocation, width, height);

  return true;
}

/**
 * Is called when window is resized
 */
void windowSizeCallback(FlutterEngine &engine, int width, int height)
{

  FlutterWindowMetricsEvent metrics_event = {};
  metrics_event.struct_size = sizeof(FlutterWindowMetricsEvent);
  metrics_event.width = kInitialWindowWidth;
  metrics_event.height = kInitialWindowHeight;
  metrics_event.pixel_ratio = g_pixelRatio;
  FlutterEngineSendWindowMetricsEvent(engine, &metrics_event);
}

bool runFlutter(
    const std::string &project_path,
    const std::string &icudtl_path)
{
  FlutterRendererConfig config = {};
  // Set OpenGL as rendering backend
  config.type = kSoftware;
  // Size of OpenGL config struct

  config.software.struct_size = sizeof(FlutterSoftwareRendererConfig);
  // config.software.surface_present_callback = PresentSurface2;
  config.software.surface_present_callback = PresentSurface;

  // This directory is generated by `flutter build bundle`.
  std::string assets_path = project_path + "/build/flutter_assets";
  FlutterProjectArgs args = {
      .struct_size = sizeof(FlutterProjectArgs),
      .assets_path = assets_path.c_str(),
      .icu_data_path =
          icudtl_path.c_str(), // Find this in your bin/cache directory.
  };
  engine = nullptr;
  // Run the engine
  FlutterEngineResult result =
      FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, // renderer
                       &args, nullptr, &engine);
  if (result != kSuccess || engine == nullptr)
  {
    std::cout << "Could not run the Flutter Engine." << std::endl;
    return false;
  }

  // Set focus to current window

  // Draw application on screen
  windowSizeCallback(engine, kInitialWindowWidth, kInitialWindowHeight);

  return true;
}

/**
 * Small utility function to inform the user how to write your embedder
 */
void printUsage()
{
  std::cout << "usage: embedder_example <path to project> <path to icudtl.dat>"
            << std::endl;
}

void updatePointer(FlutterPointerPhase phase, double x, double y)
{
  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = x * g_pixelRatio;
  event.y = y * g_pixelRatio;
  event.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  FlutterEngineSendPointerEvent(reinterpret_cast<FlutterEngine>(engine), &event, 1);
}

// mouse terminal support

struct termios orig_termios;
int image_width = 0;  // Width of your Sixel image in pixels
int image_height = 0; // Height of your Sixel image in pixels

void reset_terminal()
{
  printf("\033[?1003l"); // Disable all mouse events
  printf("\033[?1000l"); // Disable mouse click reporting
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void handle_signal(int sig)
{
  reset_terminal();
  exit(0);
}

void enable_raw_mode()
{
  struct termios raw;

  tcgetattr(STDIN_FILENO, &orig_termios);
  raw = orig_termios;

  // Enable raw mode
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  // Enable mouse reporting
  printf("\033[?1000h"); // Enable mouse click reporting only
}

int convert_to_pixels(int cell_pos, int cell_size, int image_size)
{
  // Convert terminal cell position to pixel position
  // cell_pos: Position in terminal cells (1-based)
  // cell_size: Size of terminal cells
  // image_size: Size of image in pixels
  return (cell_pos - 1) * image_size / cell_size;
}

// end of mouse terminal support

void sendPlatformMessage(const char *channel, const char *message)
{

  FlutterEngineResult result;
  FlutterPlatformMessage platformMessage = {0};

  platformMessage.struct_size = sizeof(FlutterPlatformMessage);
  platformMessage.channel = channel;
  platformMessage.message = reinterpret_cast<const uint8_t *>(message);
  platformMessage.message_size = strlen(message);

  printf("sending... with message:\n %s \n\n", message);

  result = FlutterEngineSendPlatformMessage(
      engine,
      &platformMessage);

  if (result != kSuccess)
  {
    printf("Error sending platform message. FlutterEngineSendPlatformMessage: %s\n", (result));
  }

  printf("got result from engine FlutterEngineSendPlatformMessage: %s\n", (result));
  // this is it!
  // std::unique_ptr<flutter::MethodChannel<std::string>> chan;
}

void process_mouse_event()
{
  char buf[6];

  // Read the remaining bytes of the mouse event
  if (read(STDIN_FILENO, buf, 3) < 3)
    return;

  int button = buf[0] - 32;
  int col = buf[1] - 32;
  int row = buf[2] - 32;

  if (button == 0)
  { // Left click
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    // printf("Mouse Click at Char (X=%d, Y=%d)\n", col, row);

    double pixels_per_col = w.ws_xpixel / (double)w.ws_col;
    double pixels_per_row = w.ws_ypixel / (double)w.ws_row;

    int pixel_x = (int)(col * pixels_per_col);
    int pixel_y = (int)(row * pixels_per_row);

    updatePointer(FlutterPointerPhase::kDown, pixel_x / g_pixelRatio, pixel_y / g_pixelRatio);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    updatePointer(FlutterPointerPhase::kUp, pixel_x / g_pixelRatio, pixel_y / g_pixelRatio);
  }
}

void process_keyboard_event(char ch)
{
  printf("You pressed: %c (ASCII: %d)\n", ch, ch);
}

int main(int argc, const char *argv[])
{
  if (argc != 3)
  {
    printUsage();
    return 1;
  }

  std::string project_path = argv[1];
  std::string icudtl_path = argv[2];

  bool result = runFlutter(project_path, icudtl_path);
  if (!result)
  {
    std::cout << "Could not run the Flutter engine." << std::endl;
    return EXIT_FAILURE;
  }
  printf("\033[2J\033[H");

  // Set up signal handling
  signal(SIGINT, handle_signal);

  // // Enable raw mode and mouse reporting
  enable_raw_mode();

  // mouse support
  char buf[6];
  char ch;

  while (true)
  {
    if (read(STDIN_FILENO, &buf[0], 1) < 1)
      continue;

    if (buf[0] == ESC)
    { // Escape sequence (potential mouse or special key)
      if (read(STDIN_FILENO, &buf[1], 1) < 1)
        continue;

      if (buf[1] == '[')
      { // Possible mouse or arrow key
        if (read(STDIN_FILENO, &buf[2], 1) < 1)
          continue;

        if (buf[2] == 'M')
        { // Mouse event
          process_mouse_event();
          continue;
        }
      }
    }
    else
    { // Regular key press
      process_keyboard_event(buf[0]);
      if (buf[0] == 'q')
        break; // Exit on 'q'
    }
  }

  reset_terminal();
  std::cout << "Hello, CMake!" << std::endl;
  return 0;
}