// Color definition taken from st7735.h
#define col565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))
#define c000 col565(0, 0, 0)
#define c001 col565(0, 0, 192)
#define c002 col565(0, 0, 255)

#define c010 col565(0, 192, 0)
#define c011 col565(0, 192, 192)
#define c012 col565(0, 192, 255)

#define c020 col565(0, 255, 0)
#define c021 col565(0, 255, 192)
#define c022 col565(0, 255, 255)

#define c100 col565(192, 0, 0)
#define c101 col565(192, 0, 192)
#define c102 col565(192, 0, 255)

#define c110 col565(192, 192, 0)
#define c111 col565(192, 192, 192)
#define c112 col565(192, 192, 255)

#define c120 col565(192, 255, 0)
#define c121 col565(192, 255, 192)
#define c122 col565(192, 255, 255)

#define c200 col565(255, 0, 0)
#define c201 col565(255, 0, 192)
#define c202 col565(255, 0, 255)

#define c210 col565(255, 192, 0)
#define c211 col565(255, 192, 192)
#define c212 col565(255, 192, 255)

#define c220 col565(255, 255, 0)
#define c221 col565(255, 255, 192)
#define c222 col565(255, 255, 255)

// TODO write level structure
