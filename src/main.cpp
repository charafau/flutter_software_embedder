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
static double g_pixelRatio = 1.0f;
// Initial window width
static const size_t kInitialWindowWidth = 800;
// Initial window height
static const size_t kInitialWindowHeight = 600;
// Flutter View Id
static constexpr FlutterViewId kImplicitViewId = 0;

int frameNo = 0;

FlutterEngine engine = nullptr;

sixel_output_t *output;
sixel_dither_t *dither;

// bool SaveAsBMP(const char *filename, const void *buffer, int width, int height, int row_bytes)
// {
//   FILE *file = fopen(filename, "wb");
//   if (!file)
//   {
//     fprintf(stderr, "Failed to open file: %s\n", filename);
//     return false;
//   }

//   // BMP Header
//   uint8_t header[54] = {
//       0x42, 0x4D,       // Signature "BM"
//       0, 0, 0, 0,       // File size (to be filled later)
//       0, 0,             // Reserved
//       0, 0,             // Reserved
//       54, 0, 0, 0,      // Offset to pixel data
//       40, 0, 0, 0,      // Header size (40 bytes for BITMAPINFOHEADER)
//       0, 0, 0, 0,       // Width (to be filled later)
//       0, 0, 0, 0,       // Height (to be filled later)
//       1, 0,             // Planes (must be 1)
//       32, 0,            // Bits per pixel (32-bit for ARGB)
//       0, 0, 0, 0,       // Compression (0 = none)
//       0, 0, 0, 0,       // Image size (0 = no compression)
//       0x13, 0x0B, 0, 0, // Horizontal resolution (2835 pixels/meter)
//       0x13, 0x0B, 0, 0, // Vertical resolution (2835 pixels/meter)
//       0, 0, 0, 0,       // Colors in color table (0 = default)
//       0, 0, 0, 0        // Important color count (0 = all)
//   };

//   // Fill in file size
//   int file_size = 54 + (height * row_bytes);
//   header[2] = file_size & 0xFF;
//   header[3] = (file_size >> 8) & 0xFF;
//   header[4] = (file_size >> 16) & 0xFF;
//   header[5] = (file_size >> 24) & 0xFF;

//   // Fill in width and height
//   header[18] = width & 0xFF;
//   header[19] = (width >> 8) & 0xFF;
//   header[20] = (width >> 16) & 0xFF;
//   header[21] = (width >> 24) & 0xFF;

//   header[22] = height & 0xFF;
//   header[23] = (height >> 8) & 0xFF;
//   header[24] = (height >> 16) & 0xFF;
//   header[25] = (height >> 24) & 0xFF;

//   // Write the header
//   fwrite(header, sizeof(uint8_t), 54, file);

//   // Write the pixel data (BMP stores pixels bottom-to-top, so reverse rows)
//   const uint8_t *pixel_data = reinterpret_cast<const uint8_t *>(buffer);
//   for (int y = height - 1; y >= 0; y--)
//   {
//     fwrite(pixel_data + (y * row_bytes), sizeof(uint8_t), row_bytes, file);
//   }

//   fclose(file);
//   return true;
// }

// bool PresentSurface2(void *user_data,
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

  // for (size_t y = 0; y < height; y++)
  // {
  //   for (size_t x = 0; x < width; x++)
  //   {
  // size_t index = (y * width + x) * 4; // Each pixel has 4 bytes (RGBA)
  // uint8_t r = pixels[index];
  // uint8_t g = pixels[index + 1];
  // uint8_t b = pixels[index + 2];
  // uint8_t a = pixels[index + 3];

  // // printf("Pixel (%zu, %zu): R=%d, G=%d, B=%d, A=%d\n", x, y, r, g, b, a);
  // // printf("(R=%d, G=%d, B=%d, A=%d); ", r, g, b, a);
  // print_colored_text(r, g, b, "█");

  //     pixels[(y * width + x) * 3 + 0] =
  //     (unsigned char)((x + frame * 4) % 255); // Shifting Red
  // pixels[(y * width + x) * 3 + 1] =
  //     (unsigned char)((y + frame * 4) % 255); // Shifting Green
  // pixels[(y * width + x) * 3 + 2] =
  //     (unsigned char)((frame * 4) % 255); // Shifting Blue

  //   }
  //   printf("\n");
  // }

  // if (x < width && y < height)
  // {
  //   // Get pixel at (x,y)
  //   size_t index = y * width + x;
  //   uint32_t argb = pixels[index];

  //   // Extract components
  //   uint8_t a, r, g, b;
  //   GetPixelComponents(argb, &a, &r, &g, &b);
  // }

  // good first frame
  unsigned char *pixels;
  pixels = (unsigned char *)malloc(kInitialWindowWidth * kInitialWindowHeight * 4);

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      size_t index = (y * width + x) * 4;
      // pixels[(y * width + x) * 4 + 1] =
      //     (unsigned char)(local_pixels[index] % 255); // Shifting Red
      // pixels[(y * width + x) * 4 + 2] =
      //     (unsigned char)(local_pixels[index + 1] % 255); // Shifting Green
      // pixels[(y * width + x) * 4 + 3] =
      //     (unsigned char)(local_pixels[index + 2] % 255); // Shifting Blue
      // pixels[(y * width + x) * 4 + 0] =
      //     (unsigned char)(local_pixels[index + 3] % 255); // Shifting alpha

      // uint32_t argb = local_pixels[index];

      // Extract components
      // uint8_t a, r, g, b;
      // GetPixelComponents(argb, &a, &r, &g, &b);
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

  // const uint32_t *pixels = static_cast<const uint32_t *>(allocation);
  // size_t width = row_bytes / 4; // 4 bytes per pixel in ARGB

  printf("\033[H");
  sixel_encode(pixels, width, height, 0, dither, output);
  fflush(stdout);
  free(pixels);
}

bool PresentSurface(void *user_data,
                    const void *allocation,
                    size_t row_bytes,
                    size_t height)
{
  // Copy or display the buffer.
  // memcpy(display_buffer, buffer->allocation, buffer->height * buffer->row_bytes);

  // std::string fileName = std::format("pics/oout{0}.png", frameNo);
  const uint32_t *pixels = static_cast<const uint32_t *>(allocation);
  size_t width = row_bytes / 4; // 4 bytes per pixel in ARGB

  size_t x = 100; // example x coordinate
  size_t y = 100;

  // if (x < width && y < height)
  // {
  //   // Get pixel at (x,y)
  //   size_t index = y * width + x;
  //   uint32_t argb = pixels[index];

  //   // Extract components
  //   uint8_t a, r, g, b;
  //   GetPixelComponents(argb, &a, &r, &g, &b);
  // }

  print_pixels_sixel(allocation, width, height);

  // if (stbi_write_png(fileName.c_str(), kInitialWindowWidth, height, 4, allocation, kInitialWindowWidth * 4))
  // {
  //   std::cout << "Saved: " << fileName << std::endl;
  // }
  // else
  // {
  //   std::cerr << "Failed to save PNG" << std::endl;
  // }

  // frameNo += 1;

  return true;
}

/**
 * Is called when window is resized
 */
void windowSizeCallback(FlutterEngine &engine, int width, int height)
{
  FlutterWindowMetricsEvent event = {};
  // event.struct_size = sizeof(event);
  // // Real pixel count
  // event.width = width * g_pixelRatio;
  // event.height = height * g_pixelRatio;
  // event.pixel_ratio = g_pixelRatio;
  // // This example only supports a single window, therefore we assume the event
  // // occurred in the only view, the implicit view.
  // // you might have many flutter views.
  // event.view_id = kImplicitViewId;
  // FlutterEngineSendWindowMetricsEvent(
  //     engine,
  //     &event);

  FlutterWindowMetricsEvent metrics_event = {};
  metrics_event.struct_size = sizeof(FlutterWindowMetricsEvent);
  metrics_event.width = kInitialWindowWidth;
  metrics_event.height = kInitialWindowHeight;
  metrics_event.pixel_ratio = g_pixelRatio;
  FlutterEngineSendWindowMetricsEvent(engine, &metrics_event);
}

/**
 * Setups Flutter engine and runs it.
 * returns false on error.
 * GLFWwindow* window - windows pointer
 * const std::string& project_path - path to compiled flutter assets
 * const std::string& icudtl_path - TODO
 */
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

static int sixel_write(char *data, int size, void *priv)
{
  return fwrite(data, 1, size, (FILE *)priv);
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

int main(int argc, const char *argv[])
{
  if (argc != 3)
  {
    printUsage();
    return 1;
  }

  std::string project_path = argv[1];
  std::string icudtl_path = argv[2];

  sixel_output_new(&output, sixel_write, stdout, NULL);
  dither = sixel_dither_get(SIXEL_BUILTIN_XTERM256);
  sixel_dither_set_pixelformat(dither, SIXEL_PIXELFORMAT_ARGB8888);

  bool result = runFlutter(project_path, icudtl_path);
  if (!result)
  {
    std::cout << "Could not run the Flutter engine." << std::endl;
    return EXIT_FAILURE;
  }
  printf("\033[2J\033[H");

  // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  // updatePointer(FlutterPointerPhase::kDown, 770, 570);
  // std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // updatePointer(FlutterPointerPhase::kUp, 770, 570);

  // mouse support

  // Set up signal handling
  signal(SIGINT, handle_signal);

  // Enable raw mode and mouse reporting
  enable_raw_mode();

  // printf("Click on the Sixel image. Press Ctrl+C to exit.\n");
  image_width = kInitialWindowHeight;
  image_height = kInitialWindowHeight;
  // mouse support
  char buf[6];
  while (true)
  {

    if (read(STDIN_FILENO, buf, 1) == 1 && buf[0] == '\033')
    {
      if (read(STDIN_FILENO, buf + 1, 1) == 1 && buf[1] == '[')
      {
        if (read(STDIN_FILENO, buf + 2, 1) == 1 && buf[2] == 'M')
        {
          // Read the three bytes containing mouse information
          read(STDIN_FILENO, buf + 3, 3);

          // Extract button and position info
          int button = buf[3] - 32;
          int col = buf[4] - 32;
          int row = buf[5] - 32;

          // Only process mouse click events (not movement)
          if (button == 0)
          { // Left click
            // Get terminal size
            struct winsize w;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            // printf("winsize coordinates: %s\n", w);

            // Convert to pixel coordinates
            // int pixel_x = convert_to_pixels(col, w.ws_col, image_width);
            // int pixel_y = convert_to_pixels(row, w.ws_row, image_height);

            // printf("Pixel coordinates: x=%d, y=%d\n", pixel_x, pixel_y);
            // updatePointer(FlutterPointerPhase::kDown, pixel_x, pixel_y);
            // std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // updatePointer(FlutterPointerPhase::kUp, pixel_x, pixel_y);
            double pixels_per_col = w.ws_xpixel / (double)w.ws_col;
            double pixels_per_row = w.ws_ypixel / (double)w.ws_row;

            int pixel_x = (int)(col * pixels_per_col);
            int pixel_y = (int)(row * pixels_per_row);

            printf("Click at terminal cell: %d,%d\n", col, row);
            printf("Pixel position: %d,%d\n", pixel_x, pixel_y);
            printf("Terminal size: %dx%d cells, %dx%d pixels\n",
                   w.ws_col, w.ws_row, w.ws_xpixel, w.ws_ypixel);

            updatePointer(FlutterPointerPhase::kDown, pixel_x, pixel_y);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            updatePointer(FlutterPointerPhase::kUp, pixel_x, pixel_y);
          }
        }
      }
    }
  }

  // while (true)
  // {
  //   /* code */

  //   std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  //   updatePointer(FlutterPointerPhase::kDown, 770, 570);
  //   std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  //   updatePointer(FlutterPointerPhase::kUp, 770, 570);
  // }

  // int clicks = 0;

  // while (clicks < 3)
  // {
  //   // FlutterPointerEvent pointer_event = {};
  //   // pointer_event.struct_size = sizeof(FlutterPointerEvent);
  //   // pointer_event.phase = kPhaseMove;
  //   // pointer_event.physical_x = cursor_x_position;
  //   // pointer_event.physical_y = cursor_y_position;
  //   // pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  //   // pointer_event.device = 0; // mouse

  //   // Handle platform events (e.g., input).
  //   // std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS

  //   updatePointer(FlutterPointerPhase::kDown, 770, 570);
  //   updatePointer(FlutterPointerPhase::kUp, 770, 570);
  //   std::cout << "Click click!" << std::endl;

  //   clicks += 1;
  // }

  sixel_dither_unref(dither);
  sixel_output_unref(output);
  reset_terminal();
  std::cout << "Hello, CMake!" << std::endl;
  return 0;
}