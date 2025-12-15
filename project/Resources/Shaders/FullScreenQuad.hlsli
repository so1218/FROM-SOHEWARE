struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// PostEffect Mode
#define None 0
#define Grayscale 1
#define InvertColor 2
#define Sepia 3
#define Brightness 4
#define Posterization 5
#define Pixelation 6
#define ColorTint 7
#define Contrast 8
#define Saturation 9
#define HueShift 10
#define ChannelSwap 11
#define CelShading 12
#define NeonGlow 13
#define BrightExtract 14
#define Vignette 15
#define ScreenNoise 16
#define ChromaticAberration 17
#define ScreenWave 18
#define FisheyeLens 19
#define Flash 20
#define CRTScanline 21
#define BlockNoise 22
#define Solarize 23
#define MultiPosterize 24
#define RGBSplitHorizontal 25
#define InvertByY 26
#define FilmGrain 27
#define Glitch 28
#define EdgeDetection 29
#define HeatHaze 30
#define SplitToning 31
#define WaterReaction 32


// PostEffect bit flags
#define NONE                0
#define GRAYSCALE           (1 << 0)
#define INVERT_COLOR        (1 << 1)
#define SEPIA               (1 << 2)
#define BRIGHTNESS          (1 << 3)
#define POSTERIZATION       (1 << 4)
#define PIXELATION          (1 << 5)
#define COLOR_TINT          (1 << 6)
#define CONTRAST            (1 << 7)
#define SATURATION          (1 << 8)
#define HUE_SHIFT           (1 << 9)
#define CHANNEL_SWAP        (1 << 10)
#define CEL_SHADING         (1 << 11)
#define NORMAL_OUTLINE      (1 << 12)
#define BRIGHT_EXTRACT      (1 << 13)
#define VIGNETTE            (1 << 14)
#define SCREEN_NOISE        (1 << 15)
#define CHROM_ABERRATION    (1 << 16)
#define SCREEN_WAVE         (1 << 17)
#define FISHEYE             (1 << 18)
#define FLASH               (1 << 19)
#define SCANLINE            (1 << 20)
#define BLOCK_NOISE         (1 << 21)
#define SOLARIZE            (1 << 22)
#define MULTI_POSTERIZE     (1 << 23)
#define RGB_SPLIT           (1 << 24)
#define INVERT_BY_Y         (1 << 25)
#define FILM_GRAIN          (1 << 26)
#define GLITCH              (1 << 27)
#define EDGE_DETECTION      (1 << 28)
#define HEAT_HAZE           (1 << 29)
#define SPLIT_TONING        (1 << 30)
#define WATER_REFRACTION    (1 << 31)
