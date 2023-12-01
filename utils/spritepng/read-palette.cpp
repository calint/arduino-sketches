#include <cstdint>
#include <iomanip>
#include <iostream>
#include <png.h>

void printPaletteAs16BitHex(const char *filename) {
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

  // Ensure the PNG is paletted
  if (png_get_color_type(png, info) != PNG_COLOR_TYPE_PALETTE) {
    std::cerr << "Error: The PNG is not paletted." << std::endl;
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return;
  }

  // Get the palette
  png_colorp palette;
  int numPaletteEntries;
  png_get_PLTE(png, info, &palette, &numPaletteEntries);

  // Convert RGB values to 16-bit hex
  for (int i = 0; i < numPaletteEntries; i++) {
    uint16_t rgb565 = ((palette[i].red & 0xF8) << 8) |
                      ((palette[i].green & 0xFC) << 3) | (palette[i].blue >> 3);
    // std::cout << "0x" << std::hex << rgb565 << std::endl;
    std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
              << (rgb565 & 0xff) << std::setw(2) << std::setfill('0')
              << ((rgb565 >> 8) & 0xff) << "," << std::endl;
  }

  // Clean up
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "usage: read-palette <filename>" << std::endl;
    return 1;
  }
  printPaletteAs16BitHex(argv[1]);
  return 0;
}
