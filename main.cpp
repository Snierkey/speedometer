#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <cmath>
#include <cstdint>
#include <cstdio>

MYMODCFG(net.local.gtasa.speedometer, Speedometer, 1.0, Local)

static const float POS_X_PERCENT = 0.90f;   // of screen width
static const float POS_Y_PERCENT = 0.86f;   // of screen height
static const float TEXT_SCALE_X  = 0.55f;   // in 640x448 units
static const float TEXT_SCALE_Y  = 1.10f;
static const int   FONT_STYLE    = 1;       // 1 = pricedown style
static const float KMH_FACTOR    = 180.0f;  // units/frame -> km/h

static uintptr_t pGTASA;
static void*     hGTASA;

typedef void* (*FindPlayerVehicle_t)(int, bool);
typedef float* (*FindPlayerSpeed_t)(int);
typedef void  (*PrintString_t)(float, float, unsigned short*);
typedef void  (*SetScale_t)(float, float);
typedef void  (*SetFontStyle_t)(short);
typedef void  (*SetColor_t)(uint32_t);
typedef void  (*SetEdge_t)(short);
typedef void  (*SetJustify_t)(bool);
typedef void  (*RenderFontBuffer_t)();

static FindPlayerVehicle_t FindPlayerVehicle;
static FindPlayerSpeed_t   FindPlayerSpeed;
static PrintString_t       PrintString;
static SetScale_t          SetScale;
static SetFontStyle_t      SetFontStyle;
static SetColor_t          SetColor;
static SetEdge_t           SetEdge;
static SetJustify_t        SetJustify;
static RenderFontBuffer_t  RenderFontBuffer;
static int*                pRsGlobal;

static void (*CHud_Draw)();

static void Print(float x, float y, const char* s)
{
    unsigned short wide[32];
    int i = 0;
    for (; s[i] && i < 31; ++i) wide[i] = (unsigned char)s[i];
    wide[i] = 0;
    PrintString(x, y, wide);
}

static void GetScreenSize(float& w, float& h)
{
    w = 1920.0f; h = 1080.0f;
    if (!pRsGlobal) return;
    for (int i = 1; i < 7; ++i)
    {
        int a = pRsGlobal[i], b = pRsGlobal[i + 1];
        if (a >= 200 && a <= 10000 && b >= 200 && b <= 10000)
        {
            w = (float)a; h = (float)b;
            return;
        }
    }
}

static void DrawSpeedometer()
{
    if (!FindPlayerVehicle(-1, false)) return;

    float* v = FindPlayerSpeed(-1);
    if (!v) return;
    float kmh = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) * KMH_FACTOR;

    float w, h;
    GetScreenSize(w, h);
    float sx = w / 640.0f;
    float sy = h / 448.0f;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)(kmh + 0.5f));

    SetFontStyle(FONT_STYLE);
    SetJustify(false);
    SetEdge(2);
    SetScale(TEXT_SCALE_X * sx, TEXT_SCALE_Y * sy);

    SetColor(0xFFFFFFFF);
    Print(w * POS_X_PERCENT, h * POS_Y_PERCENT, buf);

    SetScale(TEXT_SCALE_X * 0.45f * sx, TEXT_SCALE_Y * 0.45f * sy);
    SetColor(0xFF00C8FF);
    Print(w * POS_X_PERCENT + 70.0f * sx * 0.55f, h * POS_Y_PERCENT + 22.0f * sy, "KM/H");

    RenderFontBuffer();
}

static void CHud_Draw_Hook()
{
    CHud_Draw();
    DrawSpeedometer();
}

ON_MOD_LOAD()
{
    logger->SetTag("Speedometer");

    pGTASA = (uintptr_t)aml->GetLib("libGTASA.so");
    hGTASA = aml->GetLibHandle("libGTASA.so");
    if (!pGTASA || !hGTASA) { logger->Error("libGTASA not found"); return; }

    FindPlayerVehicle = (FindPlayerVehicle_t)aml->GetSym(hGTASA, "_Z17FindPlayerVehicleib");
    FindPlayerSpeed   = (FindPlayerSpeed_t)  aml->GetSym(hGTASA, "_Z15FindPlayerSpeedi");
    PrintString       = (PrintString_t)      aml->GetSym(hGTASA, "_ZN5CFont11PrintStringEffPt");
    SetScale          = (SetScale_t)         aml->GetSym(hGTASA, "_ZN5CFont8SetScaleEff");
    SetFontStyle      = (SetFontStyle_t)     aml->GetSym(hGTASA, "_ZN5CFont12SetFontStyleEs");
    SetColor          = (SetColor_t)         aml->GetSym(hGTASA, "_ZN5CFont8SetColorE5CRGBA");
    SetEdge           = (SetEdge_t)          aml->GetSym(hGTASA, "_ZN5CFont7SetEdgeEs");
    SetJustify        = (SetJustify_t)       aml->GetSym(hGTASA, "_ZN5CFont10SetJustifyEb");
    RenderFontBuffer  = (RenderFontBuffer_t) aml->GetSym(hGTASA, "_ZN5CFont16RenderFontBufferEv");
    pRsGlobal         = (int*)               aml->GetSym(hGTASA, "RsGlobal");

    void* hudDraw = (void*)aml->GetSym(hGTASA, "_ZN4CHud4DrawEv");

    if (!FindPlayerVehicle || !FindPlayerSpeed || !PrintString || !SetScale || !SetFontStyle ||
        !SetColor || !SetEdge || !SetJustify || !RenderFontBuffer || !hudDraw)
    {
        logger->Error("Missing symbol, mod disabled");
        return;
    }

    aml->Hook(hudDraw, (void*)CHud_Draw_Hook, (void**)&CHud_Draw);
    logger->Info("Loaded");
}
