// Organized as RGBA, little-endian byte order
static const unsigned int CGA_PALETTE[16] = {
  0xFF000000, // 0 - black
  0xFFA80000, // 1 - blue
  0xFF00A800, // 2 - green
  0xFFA8A800, // 3 - cyan
  0xFF0000A8, // 4 - red
  0xFFA800A8, // 5 - magenta
  0xFF00A8A8, // 6 - brown
  0xFFA8A8A8, // 7 - light gray
  0xFFA8A8A8, // 8 - gray
  0xFFFFA8A8, // 9 - light blue
  0xFFA8FFA8, // 10 - light green
  0xFFFFFFA8, // 11 - light cyan
  0xFFA8A8FF, // 12 - light red
  0xFFFFA8FF, // 13 - light magenta
  0xFFA8FFFF, // 14 - yellow
  0xFFFFFFFF  // 15 - white
};

static const unsigned int CGA_GRAPHICS_PALETTE_0[8] = {
  0xFF000000, // 0000 - black
  0xFF000000, // 0001 - black
  0xFF00A800, // 0010 - green
  0xFFA8FFA8, // 0011 - light green
  0xFF0000A8, // 0100 - red
  0xFFA8A8FF, // 0101 - light red
  0xFF00A8A8, // 0111 - yellow
  0xFF00FFFF, // 0110 - light yellow
};

static const unsigned int CGA_GRAPHICS_PALETTE_1[8] = {
  0xFF000000, // 0000 - black
  0xFF000000, // 0001 - black
  0xFFA8A800, // 0010 - cyan
  0xFFFFFFA8, // 0011 - light cyan
  0xFFA800A8, // 0100 - magenta
  0xFFFFA8FF, // 0101 - light magenta
  0xFFA8A8A8, // 0110 - gray
  0xFFFFFFFF  // 0111 - white
};

static const unsigned int CGA_GRAPHICS_PALETTE_2[8] = {
  0xFF000000, // 0000 - black
  0xFF000000, // 0001 - black
  0xFF0000A8, // 0010 - red
  0xFFA8A8FF, // 0011 - light red
  0xFFA8A800, // 0100 - cyan
  0xFFFFFFA8, // 0101 - light cyan
  0xFFA8A8A8, // 0110 - gray
  0xFFFFFFFF  // 0111 - white
};