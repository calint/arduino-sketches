#include <iomanip>
#include <iostream>
#include <png.h>

void readPalettedPNG(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    std::cerr << "Error: Unable to open file for reading." << std::endl;
    return;
  }

  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    std::cerr << "Error: Unable to create PNG read struct." << std::endl;
    fclose(fp);
    return;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    std::cerr << "Error: Unable to create PNG info struct." << std::endl;
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(fp);
    return;
  }

  png_init_io(png, fp);
  png_read_info(png, info);

  int width = png_get_image_width(png, info);
  int height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth = png_get_bit_depth(png, info);
  //   std::cout << "size " << width << "x" << height
  //             << "  bit depth: " << (int)bit_depth << std::endl;

  // Ensure the PNG is paletted
  if (color_type != PNG_COLOR_TYPE_PALETTE) {
    std::cerr << "Error: The PNG is not paletted." << std::endl;
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return;
  }

  // Read the image data
  png_bytep row_pointers[height];
  for (int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
  }
  png_read_image(png, row_pointers);

  for (int dx = 0; dx < 16; dx++) {
    // Extract a 16x16 region and convert to hex
    std::cout << std::dec << "{ // " << dx << std::endl;
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        // Access the pixel data at (x, y) and convert to hex
        png_byte pixel = row_pointers[y][dx * 16 + x];
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(pixel) << ",";
      }
      std::cout << std::endl;
    }
    std::cout << "},";
  }
  std::cout << std::endl;

  // Clean up
  for (int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "usage: read-palette <filename>" << std::endl;
    return 1;
  }
  readPalettedPNG(argv[1]);
  return 0;
}
