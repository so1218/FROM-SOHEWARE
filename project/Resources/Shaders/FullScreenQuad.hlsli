struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// PostEffect bit flags
#define NONE                0
#define GRAYSCALE           (1 << 0)
#define SEPIA               (1 << 1)
#define PIXELATION          (1 << 2)
#define COLOR_TINT          (1 << 3)
#define CEL_SHADING         (1 << 4)
#define VIGNETTE            (1 << 5)
#define SCREEN_NOISE        (1 << 6)
#define CHROM_ABERRATION    (1 << 7)
#define SCREEN_WAVE         (1 << 8)
#define FISHEYE             (1 << 9)
#define SCANLINE            (1 << 10)
#define BLOCK_NOISE         (1 << 11)
#define RGB_SPLIT           (1 << 12)
#define FILM_GRAIN          (1 << 13)
#define GLITCH              (1 << 14)
#define HEAT_HAZE           (1 << 15)
#define WATER_REFRACTION    (1 << 16)
