/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║           ESP32 HW DIAGNOSTICS  —  Hardware Suite v1.0       ║
 * ║                  by Patrick Perez  © 2026                    ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  ESP32-S3  │  ES8311 Codec (speaker)  │  INMP441 Mic         ║
 * ║  ST7789 TFT 240×320 (landscape → 320×240)                    ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * ── WIRING ─────────────────────────────────────────────────────
 *  ES8311 codec : PA_EN→48  DOUT→45  DIN→12  WS→13
 *                 BCLK→14   MCLK→38  SCL→2   SDA→1
 *  INMP441 mic  : VDD→3.3V  GND→GND  L/R→GND
 *                 WS→4      SCK→5    SD→6
 *  TFT ST7789   : DC→39  CS→47  CLK→41  SDA→40  BLK→42
 *
 * ── LIBRARIES ──────────────────────────────────────────────────
 *  arduino-audio-tools  (pschatzmann/arduino-audio-tools)
 *  arduino-audio-driver (pschatzmann/arduino-audio-driver)
 *  Adafruit ST7789  +  Adafruit GFX Library
 *
 * ── BOARD SETTINGS ─────────────────────────────────────────────
 *  Board: ESP32S3 Dev Module   USB CDC on Boot: Enabled
 */

#include "AudioTools.h"
#include "AudioTools/AudioLibs/I2SCodecStream.h"
#include "AudioTools/CoreAudio/AudioI2S/I2SStream.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ═══════════════════════════════════════════════════════════════
//  PIN DEFINITIONS
// ═══════════════════════════════════════════════════════════════
#define PIN_SDA       1
#define PIN_SCL       2
#define PIN_MCLK      38
#define PIN_BCLK      14
#define PIN_WS        13
#define PIN_DOUT      45
#define PIN_DIN       12
#define PIN_PA_EN     48
#define ES8311_ADDR   0x18

#define PIN_MIC_WS    4
#define PIN_MIC_SCK   5
#define PIN_MIC_SD    6

#define PIN_TFT_CS    47
#define PIN_TFT_DC    39
#define PIN_TFT_RST   -1
#define PIN_TFT_BLK   42
#define PIN_TFT_CLK   41
#define PIN_TFT_MOSI  40

// ═══════════════════════════════════════════════════════════════
//  AUDIO CONFIG
// ═══════════════════════════════════════════════════════════════
#define SAMPLE_RATE   16000
#define CHANNELS      2
#define BITS          16

AudioInfo                     audio_info(SAMPLE_RATE, CHANNELS, BITS);
SineWaveGenerator<int16_t>    sine_wave(32000);
GeneratedSoundStream<int16_t> sound_stream(sine_wave);
DriverPins                    my_pins;
AudioBoard                    audio_board(AudioDriverES8311, my_pins);
I2SCodecStream                i2s_stream(audio_board);
StreamCopy                    copier(i2s_stream, sound_stream);
I2SStream                     mic_stream;

// ═══════════════════════════════════════════════════════════════
//  DISPLAY + PALETTE
// ═══════════════════════════════════════════════════════════════
SPIClass tftSPI(HSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&tftSPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

#define SCR_W   320
#define SCR_H   240

#define C_BG        0x0841
#define C_PANEL     0x1082
#define C_BORDER    0x2945
#define C_CYAN      0x07FF
#define C_LIME      0x87E0
#define C_RED       0xF820
#define C_ORG       0xFD40
#define C_YEL       0xFFE0
#define C_WHT       0xFFFF
#define C_DIM       0x4228
#define C_TEAL      0x0410

static void hRule(int y, uint16_t c = C_BORDER) {
    tft.drawFastHLine(0, y, SCR_W, c);
}

// ═══════════════════════════════════════════════════════════════
//  BOOT ANIMATION
// ═══════════════════════════════════════════════════════════════
static void star(int x, int y, uint16_t c) {
    tft.drawPixel(x, y, c);
}

static void bootAnimation() {
    // --- Phase 1: Starfield sweep ---
    tft.fillScreen(0x0000);
    for (int i = 0; i < 120; i++) {
        int x = random(SCR_W), y = random(SCR_H);
        uint16_t br = random(3);
        uint16_t c = br == 0 ? 0x2104 : br == 1 ? 0x4208 : 0x7BEF;
        star(x, y, c);
        delay(4);
    }
    delay(100);

    // --- Phase 2: Horizontal scan lines wipe in ---
    for (int y = 0; y < SCR_H; y += 2) {
        tft.drawFastHLine(0, y, SCR_W, C_TEAL);
        if (y % 12 == 0) delay(2);
    }
    delay(120);
    tft.fillScreen(C_BG);

    // --- Phase 3: Corner bracket animation ---
    int cx = SCR_W / 2, cy = SCR_H / 2;
    int bLen = 24;
    for (int r = 10; r <= 72; r += 4) {
        tft.drawRect(cx - r + 1, cy - r + 1, (r - 1) * 2, (r - 1) * 2, C_BG);
        tft.drawFastHLine(cx - r,        cy - r,        bLen, C_CYAN);
        tft.drawFastVLine(cx - r,        cy - r,        bLen, C_CYAN);
        tft.drawFastHLine(cx + r - bLen, cy - r,        bLen, C_CYAN);
        tft.drawFastVLine(cx + r,        cy - r,        bLen, C_CYAN);
        tft.drawFastHLine(cx - r,        cy + r,        bLen, C_CYAN);
        tft.drawFastVLine(cx - r,        cy + r - bLen, bLen, C_CYAN);
        tft.drawFastHLine(cx + r - bLen, cy + r,        bLen, C_CYAN);
        tft.drawFastVLine(cx + r,        cy + r - bLen, bLen, C_CYAN);
        delay(28);
    }

    // --- Phase 4: Logo text fade in ---
    // textSize(3): each char = 18px wide x 24px tall. "ESP32 HW" = 8 chars = 144px -> cx-72
    // textSize(2): each char = 12px wide x 16px tall. "DIAGNOSTICS" = 11 chars = 132px -> cx-66
    // Solid dark backing rect so cyan text is visible against BG/bracket colour
    tft.fillRect(cx - 80, cy - 30, 160, 56, C_BG);

    uint16_t fadeColors[] = { 0x0410, 0x0451, 0x04B2, 0x0593, 0x06D4, 0x07FF };
    for (int f = 0; f < 6; f++) {
        tft.fillRect(cx - 80, cy - 30, 160, 56, C_BG); // restamp each pass
        tft.setTextColor(fadeColors[f]);
        tft.setTextSize(3);
        tft.setCursor(cx - 72, cy - 28);
        tft.print("ESP32 HW");
        tft.setTextSize(2);
        tft.setCursor(cx - 66, cy + 2);
        tft.print("DIAGNOSTICS");
        delay(60);
    }

    tft.setTextColor(C_WHT);
    tft.setTextSize(2);
    tft.setCursor(cx - 96, cy + 28);
    tft.print("by Patrick Perez");
    delay(400);

    // Version badge — 96px wide, centred
    tft.fillRoundRect(cx - 48, cy + 52, 96, 16, 4, C_CYAN);
    tft.setTextColor(C_BG);
    tft.setTextSize(1);
    tft.setCursor(cx - 42, cy + 56);
    tft.print("v1.0  ESP32-S3");
    delay(700);

    // --- Phase 5: Wipe out ---
    for (int y = 0; y <= SCR_H; y += 6) {
        tft.fillRect(0, y, SCR_W, 6, C_BG);
        delay(6);
    }
}

// ═══════════════════════════════════════════════════════════════
//  MAIN UI LAYOUT
// ═══════════════════════════════════════════════════════════════
#define HDR_H       18
#define BADGE_Y     20
#define BADGE_H     20
#define LABEL_Y     42
#define PROG_Y      60
#define PROG_H      8
#define SCOPE_Y     74
#define SCOPE_H     90    // was 100 — leaves a clean gap above LOG_Y
#define LOG_Y       178
#define LOG_H       14
#define LOG_LINES   3
#define STAT_Y      228
#define STAT_H      12

static char logBuf[LOG_LINES][48];
static int  logHead = 0;

static void drawHeader() {
    tft.fillRect(0, 0, SCR_W, HDR_H, C_PANEL);
    hRule(HDR_H, C_CYAN);
    tft.setTextColor(C_CYAN); tft.setTextSize(1);
    tft.setCursor(4, 5);  tft.print("ESP32 HW DIAGNOSTICS");
    tft.setTextColor(C_DIM);
    tft.setCursor(196, 5); tft.print("Hardware Suite v1.0");
    tft.fillCircle(188, 9, 3, C_LIME);
}

static uint8_t badgeState[3] = {0, 0, 0};
static const char* badgeNames[] = {"DISPLAY", "CODEC", "MIC"};
static void drawBadges() {
    int bx = 4;
    tft.fillRect(0, BADGE_Y, SCR_W, BADGE_H, C_BG);
    for (int i = 0; i < 3; i++) {
        uint16_t bc = badgeState[i] == 0 ? C_BORDER :
                      badgeState[i] == 1 ? C_LIME   : C_RED;
        tft.fillRoundRect(bx, BADGE_Y+2, 94, 16, 4, C_PANEL);
        tft.drawRoundRect(bx, BADGE_Y+2, 94, 16, 4, bc);
        tft.setTextColor(bc); tft.setTextSize(1);
        tft.setCursor(bx + 4, BADGE_Y + 6); tft.print(badgeNames[i]);
        tft.setCursor(bx + 60, BADGE_Y + 6);
        if      (badgeState[i] == 0) { tft.setTextColor(C_DIM);  tft.print("..."); }
        else if (badgeState[i] == 1) { tft.setTextColor(C_LIME); tft.print("PASS"); }
        else                         { tft.setTextColor(C_RED);  tft.print("FAIL"); }
        bx += 100;
    }
}

static void drawTestLabel(const char* lbl, uint16_t col = C_CYAN) {
    tft.fillRect(0, LABEL_Y, SCR_W, 14, C_BG);
    tft.setTextColor(col); tft.setTextSize(1);
    tft.setCursor(4, LABEL_Y + 2);
    tft.print("\x10 ");
    tft.print(lbl);
}

static void drawProgress(int step, int total) {
    tft.fillRect(0, PROG_Y, SCR_W, PROG_H, C_PANEL);
    tft.drawRect(0, PROG_Y, SCR_W, PROG_H, C_BORDER);
    int filled = (step * (SCR_W - 2)) / total;
    if (filled > 0)
        tft.fillRect(1, PROG_Y + 1, filled, PROG_H - 2, C_CYAN);
    char pct[8]; snprintf(pct, 8, "%d%%", (step * 100) / total);
    tft.setTextColor(C_BG); tft.setTextSize(1);
    int tx = 1 + filled - 22;
    if (tx < 1) tx = 2;
    tft.setCursor(tx, PROG_Y + 1); tft.print(pct);
}

static void drawLog() {
    tft.fillRect(0, LOG_Y, SCR_W, LOG_LINES * LOG_H, C_BG);
    hRule(LOG_Y, C_BORDER);
    for (int i = 0; i < LOG_LINES; i++) {
        int idx = (logHead + i) % LOG_LINES;
        int age = LOG_LINES - 1 - i;
        uint16_t col = age == 0 ? C_WHT :
                       age == 1 ? 0xC618 :
                       age == 2 ? 0x8410 : C_DIM;
        tft.setTextColor(col); tft.setTextSize(1);
        tft.setCursor(4, LOG_Y + i * LOG_H + 2);
        tft.print(logBuf[idx]);
    }
}

static void logMsg(const char* fmt, ...) {
    char b[48]; va_list a; va_start(a, fmt); vsnprintf(b, 48, fmt, a); va_end(a);
    strncpy(logBuf[logHead], b, 47);
    logHead = (logHead + 1) % LOG_LINES;
    drawLog();
}

static void drawStatus(const char* msg, uint16_t col = C_DIM) {
    tft.fillRect(0, STAT_Y, SCR_W, STAT_H, C_PANEL);
    hRule(STAT_Y, C_BORDER);
    tft.setTextColor(col); tft.setTextSize(1);
    tft.setCursor(4, STAT_Y + 2); tft.print(msg);
    tft.setTextColor(C_DIM);
    tft.setCursor(230, STAT_Y + 2); tft.print("P.Perez 2026");
}

// ─── Scope ────────────────────────────────────────────────────
#define SCOPE_W  200
static int16_t scopeBuf[SCOPE_W] = {};
static int     scopeHead = 0;

static void drawScope(int16_t peak = 0, bool clear = false) {
    int sx = 0, sy = SCOPE_Y, sw = SCR_W, sh = SCOPE_H;
    if (clear) { tft.fillRect(sx, sy, sw, sh, C_BG); return; }
    tft.fillRect(sx, sy, sw, sh, C_BG);
    hRule(sy, C_BORDER);
    hRule(sy + sh - 1, C_BORDER);
    tft.drawFastHLine(sx, sy + sh / 2, sw, C_TEAL);
    for (int gx = 0; gx < sw; gx += 32)
        tft.drawFastVLine(sx + gx, sy + 1, sh - 2, C_TEAL);
    for (int x = 0; x < SCOPE_W - 1; x++) {
        int i0 = (scopeHead + x)     % SCOPE_W;
        int i1 = (scopeHead + x + 1) % SCOPE_W;
        int y0 = sy + sh/2 - (int)((long)scopeBuf[i0] * (sh/2 - 6) / 32768);
        int y1 = sy + sh/2 - (int)((long)scopeBuf[i1] * (sh/2 - 6) / 32768);
        y0 = constrain(y0, sy+1, sy+sh-2);
        y1 = constrain(y1, sy+1, sy+sh-2);
        int px  = sx + x       * sw / SCOPE_W;
        int px2 = sx + (x + 1) * sw / SCOPE_W;
        tft.drawLine(px, y0, px2, y1, C_YEL);
    }
    char lbl[24]; snprintf(lbl, 24, "PEAK %d", abs(peak));
    tft.fillRect(sw - 72, sy + 2, 70, 11, C_BG);
    tft.setTextColor(abs(peak) > 1000 ? C_LIME : C_YEL);
    tft.setTextSize(1);
    tft.setCursor(sw - 70, sy + 3);
    tft.print(lbl);
}

// ─── Colour-wash test ─────────────────────────────────────────
static void displayColorTest() {
    struct { uint16_t c; const char* n; } cols[] = {
        {C_RED,  "RED"  }, {0x07E0, "GRN"  }, {0x001F, "BLU"  },
        {C_CYAN, "CYAN" }, {C_YEL,  "YEL"  }, {C_WHT,  "WHT"  },
    };
    for (auto& col : cols) {
        tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, col.c);
        tft.setTextColor(C_BG); tft.setTextSize(2);
        tft.setCursor(12, SCOPE_Y + SCOPE_H / 2 - 10);
        tft.print(col.n);
        delay(250);
    }
    tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, C_BG);
    for (int x = 0; x < SCR_W; x++) {
        uint8_t r = map(x, 0, SCR_W - 1, 0, 31);
        uint8_t g = map(x, 0, SCR_W - 1, 0, 63);
        uint8_t b = map(x, 0, SCR_W - 1, 31, 0);
        uint16_t gc = (r << 11) | (g << 5) | b;
        tft.drawFastVLine(x, SCOPE_Y + 8, SCOPE_H - 16, gc);
    }
    delay(400);
    tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, C_BG);
    for (int py = SCOPE_Y + 4; py < SCOPE_Y + SCOPE_H - 4; py += 4)
        for (int px2 = 4; px2 < SCR_W - 4; px2 += 4)
            tft.drawPixel(px2, py, (px2 + py) % 3 == 0 ? C_CYAN : C_DIM);
    delay(300);
}

// ═══════════════════════════════════════════════════════════════
//  AUDIO HELPERS
// ═══════════════════════════════════════════════════════════════
static void playFreq(float freq, int ms) {
    sine_wave.setFrequency(freq);
    uint32_t until = millis() + ms;
    while (millis() < until) { copier.copy(); yield(); }
}

static int16_t recordMic(int ms) {
    int32_t  buf[256];
    int16_t  peak = 0;
    uint32_t until = millis() + ms;
    int      tick  = 0;
    while (millis() < until) {
        int rd = mic_stream.readBytes((uint8_t*)buf, sizeof(buf));
        int samples = rd / 4;
        for (int i = 0; i < samples; i++) {
            int16_t s = (int16_t)(buf[i] >> 14);
            if (abs(s) > abs(peak)) peak = s;
            scopeBuf[scopeHead % SCOPE_W] = s;
            scopeHead++;
        }
        if ((tick++ & 0x1F) == 0) drawScope(peak);
        yield();
    }
    return peak;
}

static void chime() {
    int freqs[] = {523, 659, 784, 1047};
    for (int f : freqs) { playFreq(f, 120); delay(30); }
}

// ═══════════════════════════════════════════════════════════════
//  SUMMARY SCREEN LIVE SCOPE
// ═══════════════════════════════════════════════════════════════
// Layout on summary page:
//   Header  0..22
//   Row 0  30..68   (Display)
//   Row 1  74..112  (Codec)
//   Row 2 118..156  (Mic)
//   Live scope 160..222  ← NEW
//   Footer 228..240

#define SUM_SCOPE_Y   160
#define SUM_SCOPE_H    60
#define SUM_SCOPE_W   200   // same ring buffer width

static int16_t sumScopeBuf[SUM_SCOPE_W] = {};
static int     sumScopeHead = 0;

static void drawSummaryScope(int16_t peak) {
    int sx = 0, sy = SUM_SCOPE_Y, sw = SCR_W, sh = SUM_SCOPE_H;
    tft.fillRect(sx, sy, sw, sh, C_BG);
    // Grid
    tft.drawFastHLine(sx, sy,        sw, C_BORDER);
    tft.drawFastHLine(sx, sy+sh-1,   sw, C_BORDER);
    tft.drawFastHLine(sx, sy+sh/2,   sw, C_TEAL);
    for (int gx = 0; gx < sw; gx += 32)
        tft.drawFastVLine(sx+gx, sy+1, sh-2, C_TEAL);
    // Label
    tft.setTextColor(C_YEL); tft.setTextSize(1);
    tft.setCursor(sx+4, sy+3);
    tft.print("LIVE MIC");
    char pl[20]; snprintf(pl, 20, "pk:%d", abs(peak));
    tft.setCursor(sx+70, sy+3); tft.print(pl);
    // Waveform
    for (int x = 0; x < SUM_SCOPE_W - 1; x++) {
        int i0 = (sumScopeHead + x)     % SUM_SCOPE_W;
        int i1 = (sumScopeHead + x + 1) % SUM_SCOPE_W;
        int y0 = sy+sh/2 - (int)((long)sumScopeBuf[i0]*(sh/2-6)/32768);
        int y1 = sy+sh/2 - (int)((long)sumScopeBuf[i1]*(sh/2-6)/32768);
        y0 = constrain(y0, sy+1, sy+sh-2);
        y1 = constrain(y1, sy+1, sy+sh-2);
        int px  = sx + x       * sw / SUM_SCOPE_W;
        int px2 = sx + (x+1)   * sw / SUM_SCOPE_W;
        tft.drawLine(px, y0, px2, y1, C_YEL);
    }
}

// ═══════════════════════════════════════════════════════════════
//  FINAL SUMMARY SCREEN
// ═══════════════════════════════════════════════════════════════
static void drawSummary(bool dispOk, bool codecOk, bool micOk, int16_t micPeak) {
    tft.fillScreen(C_BG);
    bool allPass = dispOk && codecOk && micOk;

    // Header
    uint16_t hCol = allPass ? C_LIME : C_ORG;
    tft.fillRect(0, 0, SCR_W, 22, allPass ? 0x0420 : 0x2000);
    hRule(22, hCol);
    tft.setTextColor(hCol); tft.setTextSize(2);
    tft.setCursor(4, 4);
    tft.print(allPass ? "ALL TESTS PASSED" : "TESTS COMPLETE");

    // Result rows — 3 rows × 44px = 132px → 30..162, fits above live scope
    struct Row { const char* name; bool ok; const char* detail; };
    char micDetail[24]; snprintf(micDetail, 24, "peak = %d", abs(micPeak));
    Row rows[] = {
        { "Display", dispOk,  dispOk  ? "Colors / pixels OK"  : "Check TFT wiring"   },
        { "Codec",   codecOk, codecOk ? "ES8311 DAC / PA OK"  : "Check codec wiring"  },
        { "Mic",     micOk,   micDetail },
    };
    int ry = 28;
    for (auto& row : rows) {
        uint16_t rc = row.ok ? C_LIME : C_RED;
        tft.fillRect(4, ry, SCR_W-8, 36, C_PANEL);
        tft.drawRect(4, ry, SCR_W-8, 36, rc);
        tft.fillRect(4, ry, 4, 36, rc);           // left accent bar
        tft.setTextColor(C_WHT); tft.setTextSize(2);
        tft.setCursor(14, ry+3); tft.print(row.name);
        tft.setTextColor(C_DIM); tft.setTextSize(1);
        tft.setCursor(14, ry+22); tft.print(row.detail);
        tft.fillRoundRect(SCR_W-58, ry+9, 54, 16, 4, rc);
        tft.setTextColor(C_BG); tft.setTextSize(1);
        tft.setCursor(SCR_W-46, ry+13);
        tft.print(row.ok ? "  PASS" : "  FAIL");
        ry += 42;
    }
    // ry ends at ~154, SUM_SCOPE_Y starts at 160 — clean 6px gap

    // Draw empty scope placeholder
    drawSummaryScope(0);

    hRule(STAT_Y, C_BORDER);
    tft.fillRect(0, STAT_Y, SCR_W, STAT_H, C_PANEL);
    tft.setTextColor(C_DIM); tft.setTextSize(1);
    tft.setCursor(4, STAT_Y+2);
    tft.print("ESP32 HW Diagnostics v1.0  \xb7  Patrick Perez 2026");
}

// ═══════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════
static bool allDone  = false;
static bool micReady = false;

void setup() {
    pinMode(PIN_TFT_BLK, OUTPUT); digitalWrite(PIN_TFT_BLK, HIGH);
    pinMode(PIN_PA_EN,   OUTPUT); digitalWrite(PIN_PA_EN, LOW);

    tftSPI.begin(PIN_TFT_CLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
    tft.init(240, 320);
    tft.setRotation(3);
    tft.fillScreen(0x0000);

    bootAnimation();

    tft.fillScreen(C_BG);
    drawHeader();
    drawBadges();
    drawTestLabel("Initialising...");
    drawProgress(0, 10);
    drawScope(0, true);
    memset(logBuf, 0, sizeof(logBuf));
    drawLog();
    drawStatus("System startup", C_DIM);

    // ════════════════════════════════════════════════════════════
    //  TEST 1 — DISPLAY
    // ════════════════════════════════════════════════════════════
    drawTestLabel("TEST 1 / 3  \x10  Display", C_CYAN);
    drawProgress(1, 10);
    logMsg("[DISPLAY] Starting colour test...");
    delay(300);

    displayColorTest();

    tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, C_BG);
    drawScope(0);
    badgeState[0] = 1;
    drawBadges();
    drawProgress(3, 10);
    logMsg("[DISPLAY] Colors / pixels  PASS");
    drawStatus("Display OK", C_LIME);
    delay(400);

    // ════════════════════════════════════════════════════════════
    //  TEST 2 — CODEC / SPEAKER
    // ════════════════════════════════════════════════════════════
    drawTestLabel("TEST 2 / 3  \x10  Codec + Speaker", C_CYAN);
    drawProgress(4, 10);
    logMsg("[CODEC] Starting ES8311 init...");

    Wire.begin(PIN_SDA, PIN_SCL, 100000);
    delay(50);
    my_pins.addI2C(PinFunction::CODEC, PIN_SCL, PIN_SDA, ES8311_ADDR, 100000, Wire);
    my_pins.addI2S(PinFunction::CODEC, PIN_MCLK, PIN_BCLK, PIN_WS, PIN_DOUT, PIN_DIN);
    my_pins.addPin(PinFunction::PA, PIN_PA_EN, PinLogic::Output);

    auto cfg = i2s_stream.defaultConfig(TX_MODE);
    cfg.copyFrom(audio_info);
    cfg.output_device = DAC_OUTPUT_ALL;
    bool codecOk = i2s_stream.begin(cfg);
    i2s_stream.setVolume(0.3);

    if (!codecOk) {
        badgeState[1] = 2;
        drawBadges();
        logMsg("[CODEC] Init FAILED");
        drawStatus("Codec init failed — check wiring", C_RED);
    } else {
        logMsg("[CODEC] ES8311 ready");

        auto sineCfg = sine_wave.defaultConfig();
        sineCfg.copyFrom(audio_info);
        sine_wave.begin(sineCfg);
        delay(100);

        drawTestLabel("Playing chime...", C_YEL);
        logMsg("[CODEC] Chime 523-1047Hz");
        chime();
        drawProgress(5, 10);

        drawTestLabel("Frequency sweep 200-4kHz", C_YEL);
        logMsg("[CODEC] Sweep 200Hz -> 4kHz");
        for (int i = 0; i < 20; i++) {
            float f = 200.0f + i * 190.0f;
            playFreq(f, 80);

            int bx = (i * SCR_W) / 20;
            tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, C_BG);
            tft.fillRect(0, SCOPE_Y, bx, SCOPE_H, C_TEAL);
            char fl[20]; snprintf(fl, 20, "%.0f Hz", f);
            tft.setTextColor(C_CYAN); tft.setTextSize(2);

            tft.setCursor(SCR_W/2 - 30, SCOPE_Y + SCOPE_H/2 - 8);
            tft.print(fl);
        }
        tft.fillRect(0, SCOPE_Y, SCR_W, SCOPE_H, C_BG);

        badgeState[1] = 1;
        drawBadges();
        drawProgress(6, 10);
        logMsg("[CODEC] Sweep complete  PASS");
        drawStatus("Codec OK — speaker active", C_LIME);
        delay(300);
    }

    // ════════════════════════════════════════════════════════════
    //  TEST 3 — MIC (INMP441)
    // ════════════════════════════════════════════════════════════
    drawTestLabel("TEST 3 / 3  \x10  INMP441 Mic", C_CYAN);
    drawProgress(7, 10);
    logMsg("[MIC] Starting INMP441 init...");

    auto micCfg = mic_stream.defaultConfig(RX_MODE);
    micCfg.sample_rate     = SAMPLE_RATE;
    micCfg.channels        = 2;
    micCfg.bits_per_sample = 32;
    micCfg.i2s_format      = I2S_STD_FORMAT;
    micCfg.port_no         = 1;
    micCfg.pin_ws          = PIN_MIC_WS;
    micCfg.pin_bck         = PIN_MIC_SCK;
    micCfg.pin_data        = PIN_MIC_SD;
    micCfg.pin_mck         = -1;
    micCfg.use_apll        = false;
    bool micInitOk = mic_stream.begin(micCfg);
    micReady = micInitOk;

    int16_t micPeak = 0;
    if (!micInitOk) {
        badgeState[2] = 2;
        drawBadges();
        logMsg("[MIC] Init FAILED");
        drawStatus("INMP441 init failed — check wiring", C_RED);
    } else {
        logMsg("[MIC] Recording 3s — speak now!");
        drawTestLabel("Speak into mic  (3 sec)", C_YEL);
        drawStatus("Listening...", C_YEL);
        drawProgress(8, 10);

        for (int c = 3; c >= 1; c--) {
            tft.fillRect(SCR_W-28, SCOPE_Y+2, 26, 20, C_BG);
            tft.setTextColor(C_YEL); tft.setTextSize(2);
            tft.setCursor(SCR_W-24, SCOPE_Y+4);
            tft.print(c);
            micPeak = recordMic(1000);
        }

        bool micOk = abs(micPeak) > 300;
        badgeState[2] = micOk ? 1 : 2;
        drawBadges();
        drawProgress(9, 10);
        char pl[40]; snprintf(pl, 40, "[MIC] Peak=%d  %s",
                              abs(micPeak), micOk ? "PASS" : "FAIL (too quiet?)");
        logMsg(pl);
        drawStatus(micOk ? "Mic OK" : "Mic weak — check wiring/placement",
                   micOk ? C_LIME : C_ORG);
        delay(500);
    }

    // ════════════════════════════════════════════════════════════
    //  DONE
    // ════════════════════════════════════════════════════════════
    drawProgress(10, 10);
    delay(300);

    bool allPass = (badgeState[0]==1) && (badgeState[1]==1) && (badgeState[2]==1);
    if (codecOk) {
        if (allPass) {
            int happy[] = {523, 659, 784, 1047, 1319};
            for (int f : happy) { playFreq(f, 100); delay(20); }
        } else {
            int sad[] = {440, 330, 220};
            for (int f : sad) { playFreq(f, 200); delay(30); }
        }
    }

    delay(800);
    drawSummary(badgeState[0]==1, badgeState[1]==1, badgeState[2]==1, micPeak);
    allDone = true;
}

// ═══════════════════════════════════════════════════════════════
//  LOOP — live mic scope in summary screen (below Mic row)
// ═══════════════════════════════════════════════════════════════
void loop() {
    if (!allDone) { delay(10); return; }
    if (!micReady) { delay(100); return; }

    int32_t buf[256];
    int rd = mic_stream.readBytes((uint8_t*)buf, sizeof(buf));
    int16_t pk = 0;
    for (int i = 0; i < rd/4; i++) {
        int16_t s = (int16_t)(buf[i] >> 14);
        if (abs(s) > abs(pk)) pk = s;
        sumScopeBuf[sumScopeHead % SUM_SCOPE_W] = s;
        sumScopeHead++;
    }
    if (rd > 0) drawSummaryScope(pk);

    delay(40);
}
