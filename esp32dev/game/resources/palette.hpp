// palette used to convert uint8_t to uint16_t rgb 565
// lower and higher byte swapped (red being the highest bits)
static constexpr uint16_t palette[256]{
    0b0000000000000000, // black
    0b0000000011111000, // red
    0b1110000000000111, // green
    0b0001111100000000, // blue
    0b1111111111111111, // white
};
