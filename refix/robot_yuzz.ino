/*
  Mühendisçe - Robot Yüz  (ESP32-C3 + ST7789 240x240)  --  SENSÖRSÜZ SÜRÜM

  Kütüphaneler: Adafruit GFX, Adafruit ST7789, SPI (ek kütüphane yok).
  Her şey aşağıdaki SENARYO tablosuyla otomatik oynar ve başa sarar.
  "Sallama" anında cihazı elinle gerçekten sallarsan video çok daha inandırıcı olur.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// enum EN ÜSTTE olmalı (Arduino otomatik prototip hatasını önler)
enum State { S_SLEEP, S_WAKING, S_AWAKE, S_DIZZY, S_LOGO };

// ---------- EKRAN PİNLERİ ----------
#define TFT_SCK  4
#define TFT_MOSI 6
#define TFT_RST  1
#define TFT_DC   0
#define TFT_CS   -1

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 *cv;

// ================== SENARYO (ms cinsinden, değiştirmekte özgürsün) ==================
struct Step {
  uint32_t t;          // döngü başlangıcından itibaren ms
  State    st;         // robotun durumu
  bool     drowsy;     // uykulu mu
  int8_t   lx, ly;     // bakış yönü (-22..22 , -12..12)
  const char *say;     // alt yazı (en fazla 19 karakter), yoksa nullptr
  uint16_t hold;       // alt yazının kalma süresi
  bool     happy;      // mutlu göz + yanak
};

const Step script[] = {
  {    0, S_SLEEP,  false,   0,   0, nullptr,               0, false },  // uyuyor
  { 2500, S_WAKING, false,   0,   0, "beni beğenmeyi ve",        1800, true  },  // uyanıyor
  { 3500, S_AWAKE,  false,   0,   0, nullptr,               0, false },
  { 4300, S_AWAKE,  false,   0,   0, "Merhaba, ben Refix!", 2200, false },
  { 6700, S_AWAKE,  false, -22,   6, "Etrafa bakayım...",  1800, false },  // sola kay
  { 8200, S_AWAKE,  false,  22,  -6, nullptr,               0, false },  // sağa kay
  { 9600, S_DIZZY,  false,   0,   0, "Yapma, başım döndü!", 3000, false },  // <-- cihazı salla
  {12900, S_AWAKE,  false,   0,   0, "Hh... geçti galiba.", 2200, true  },
  {15300, S_AWAKE,  true,    0,   0, "Uykum geldi...",     2200, false },  // gözler yarı kapanır
  {17300, S_SLEEP,  false,   0,   0, nullptr,               0, false },  // uyudu
  //{22300, S_LOGO,   false,   0,   0, nullptr,               0, false },  // sabit logo
};
const int NSTEPS = sizeof(script) / sizeof(script[0]);
const uint32_t CYCLE_END = 28000;     // logo bu kadar ms görünür, sonra başa sarar

// ================== RENKLER ==================
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
uint16_t C_BG, C_EYE, C_PINK, C_WHITE, C_BAR, C_GRAY, C_STAR;

uint16_t mix565(uint16_t a, uint16_t b, float t) {
  t = constrain(t, 0.f, 1.f);
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (br - ar) * t, g = ag + (bg - ag) * t, bl = ab + (bb - ab) * t;
  return (r << 11) | (g << 5) | bl;
}

// ================== TÜRKÇE METİN ==================
int decodeUtf8(const char *str, uint16_t *out, int maxn) {
  int n = 0;
  const uint8_t *p = (const uint8_t *)str;
  while (*p && n < maxn) {
    if (*p < 0x80) { out[n++] = *p++; }
    else if ((*p & 0xE0) == 0xC0 && p[1]) {
      out[n++] = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
      p += 2;
    } else p++;
  }
  return n;
}

// Varsayılan 5x7 font + Türkçe işaretler. Hücre: 6s x 8s
void drawTR(int x, int y, uint16_t cp, uint8_t s, uint16_t col) {
  char base = (char)cp;
  int mark = 0;            // 1 nokta, 2 kuyruk, 3 breve, 4 noktasız ı, 5 iki nokta
  bool up = false;
  switch (cp) {
    case 0xE7:  base = 'c'; mark = 2; break;             // ç
    case 0xC7:  base = 'C'; mark = 2; up = true; break;  // Ç
    case 0x15F: base = 's'; mark = 2; break;             // ş
    case 0x15E: base = 'S'; mark = 2; up = true; break;  // Ş
    case 0x11F: base = 'g'; mark = 3; break;             // ğ
    case 0x11E: base = 'G'; mark = 3; up = true; break;  // Ğ
    case 0x131: base = 0;   mark = 4; break;             // ı
    case 0x130: base = 'I'; mark = 1; up = true; break;  // İ
    case 0xF6:  base = 'o'; mark = 5; break;             // ö
    case 0xD6:  base = 'O'; mark = 5; up = true; break;  // Ö
    case 0xFC:  base = 'u'; mark = 5; break;             // ü
    case 0xDC:  base = 'U'; mark = 5; up = true; break;  // Ü
    default: break;
  }
  if (base) cv->drawChar(x, y, base, col, col, s);       // bg==fg -> şeffaf

  auto px = [&](int cx, int cy) { cv->fillRect(x + cx * s, y + cy * s, s, s, col); };
  switch (mark) {
    case 1: px(2, -2); break;
    case 2: px(2, 7); px(1, 8); break;
    case 3:
      if (up) { px(1, -3); px(2, -2); px(3, -3); }
      else    { px(1, -1); px(2, 0);  px(3, -1); }
      break;
    case 4:
      cv->fillRect(x + 1 * s, y + 2 * s, 2 * s, s, col);
      cv->fillRect(x + 2 * s, y + 2 * s, s, 5 * s, col);
      cv->fillRect(x + 1 * s, y + 6 * s, 3 * s, s, col);
      break;
    case 5:
      if (up) { px(1, -2); px(3, -2); }
      else    { px(1, 0);  px(3, 0); }
      break;
  }
}

void drawCPs(int x, int y, const uint16_t *cp, int n, uint8_t s, uint16_t col) {
  for (int i = 0; i < n; i++) drawTR(x + i * 6 * s, y, cp[i], s, col);
}

int centerTR(const char *str, int y, uint8_t s, uint16_t col) {
  uint16_t buf[40];
  int n = decodeUtf8(str, buf, 40);
  int x = (240 - n * 6 * s) / 2;
  drawCPs(x, y, buf, n, s, col);
  return x;
}

// ================== DURUM ==================
State state = S_SLEEP;
uint32_t stateSince = 0;

float eyeOpen = 0, lookX = 0, lookY = 0, happy = 0, cheek = 0;
float targetLX = 0, targetLY = 0;
uint32_t nextBlink = 0, blinkStart = 0, happyUntil = 0;
bool drowsy = false;

uint16_t subCp[40];
int subN = 0;
uint32_t subT0 = 0, subHold = 0;

void setState(State s) { state = s; stateSince = millis(); }

void say(const char *s, uint32_t hold = 2200) {
  subN = decodeUtf8(s, subCp, 40);
  subT0 = millis();
  subHold = hold;
}

// ================== ÇİZİM PARÇALARI ==================
void drawEye(int cx, int cy, float open, float hap, float shineShift) {
  if (open < 0.10f) {                           // kapalı göz: ‿
    cv->fillCircle(cx, cy - 2, 17, C_EYE);
    cv->fillCircle(cx, cy - 10, 17, C_BG);
    return;
  }
  float w  = 62 + (1.0f - open) * 12;
  float hh = max(6.0f, 84 * open);
  int r = min(26.0f, hh / 2);
  cv->fillRoundRect(cx - w / 2, cy - hh / 2, w, hh, r, C_EYE);

  if (hap > 0.4f) {                             // mutlu göz (^ ^)
    cv->fillCircle(cx, cy + hh / 2 + 8, w * 0.62f, C_BG);
  }
  if (open > 0.5f) {                            // parıltılar
    cv->fillCircle(cx - 12 + shineShift, cy - 20, 9, C_WHITE);
    cv->fillCircle(cx + 13 + shineShift, cy + 8, 4, C_WHITE);
  }
}

void drawSpiral(int cx, int cy, float phase, uint16_t col) {
  float px = cx, py = cy;
  for (float a = 0; a < 18.0f; a += 0.35f) {
    float r = 1.6f * a;
    float x = cx + cosf(a + phase) * r;
    float y = cy + sinf(a + phase) * r;
    cv->drawLine(px, py, x, y, col);
    cv->drawLine(px + 1, py, x + 1, y, col);
    px = x; py = y;
  }
}

void drawMouth(int cx, int my, uint32_t now) {
  if (state == S_DIZZY) {                       // dalgalı ağız
    float ph = now / 110.0f;
    for (int x = -20; x < 20; x += 2) {
      int y1 = my + sinf(x * 0.35f + ph) * 4;
      int y2 = my + sinf((x + 2) * 0.35f + ph) * 4;
      cv->drawLine(cx + x, y1, cx + x + 2, y2, C_PINK);
      cv->drawLine(cx + x, y1 + 1, cx + x + 2, y2 + 1, C_PINK);
    }
  } else if (state == S_SLEEP || drowsy) {      // nefes alan küçük "o"
    float rr = 4 + (sinf(now / 450.0f) + 1) * 2;
    cv->fillCircle(cx, my, rr, C_PINK);
  } else {                                      // gülümseme (hilal)
    cv->fillCircle(cx, my - 6, 16, C_PINK);
    cv->fillCircle(cx, my - 14, 16, C_BG);
  }
}

void drawZzz(uint32_t now) {
  const char ch[3] = { 'z', 'Z', 'Z' };
  for (int i = 0; i < 3; i++) {
    float t = fmodf(now / 1800.0f + i * 0.33f, 1.0f);
    int x = 160 + i * 20 + sinf(t * 6.28f) * 4;
    int y = 85 - t * 55 + (2 - i) * 6;
    uint16_t col = mix565(C_BG, C_GRAY, 1.0f - t);
    cv->drawChar(x, y, ch[i], col, col, 2 + i);
  }
}

void drawSubtitle(uint32_t now) {
  if (subN == 0) return;
  uint32_t typeMs = subN * 45;
  uint32_t el = now - subT0;
  if (el > typeMs + subHold) return;

  cv->fillRoundRect(2, 194, 236, 42, 12, C_BAR);
  int x = (240 - subN * 12) / 2;
  int n = min((int)(el / 45) + 1, subN);        // daktilo efekti
  drawCPs(x, 206, subCp, n, 2, C_WHITE);
}

void drawLogoScreen() {                         // SABİT kapanış ekranı
  cv->fillScreen(C_BG);
  cv->fillRoundRect(90, 30, 60, 60, 10, C_EYE);
  cv->fillRoundRect(100, 40, 40, 40, 6, C_BG);
  cv->fillRoundRect(108, 50, 6, 12, 3, C_EYE);
  cv->fillRoundRect(126, 50, 6, 12, 3, C_EYE);
  cv->fillCircle(120, 66, 7, C_EYE);
  cv->fillCircle(120, 60, 7, C_BG);
  for (int i = 0; i < 3; i++) {
    int p = 100 + i * 20;
    cv->fillRect(p - 2, 20, 4, 10, C_EYE);
    cv->fillRect(p - 2, 90, 4, 10, C_EYE);
    int q = 46 + i * 18;
    cv->fillRect(80, q - 2, 10, 4, C_EYE);
    cv->fillRect(150, q - 2, 10, 4, C_EYE);
  }

  centerTR("Mühendisçe", 118, 3, C_WHITE);
  centerTR("Bunu çocuğunla yap", 160, 2, C_GRAY);

  int total = 28 + 10 + 4 * 12;
  int x0 = (240 - total) / 2;
  int ay = 204;
  cv->drawLine(x0, ay, x0 + 24, ay, C_EYE);
  cv->drawLine(x0, ay + 1, x0 + 24, ay + 1, C_EYE);
  cv->drawLine(x0 + 24, ay, x0 + 17, ay - 6, C_EYE);
  cv->drawLine(x0 + 24, ay + 1, x0 + 17, ay + 7, C_EYE);
  cv->drawLine(x0 + 25, ay, x0 + 18, ay - 6, C_EYE);
  cv->drawLine(x0 + 25, ay + 1, x0 + 18, ay + 7, C_EYE);
  uint16_t d[4] = { 'd', 'e', 'r', 's' };
  drawCPs(x0 + 38, 196, d, 4, 2, C_EYE);
}

void drawFace(uint32_t now) {
  cv->fillScreen(C_BG);

  float wob = (state == S_DIZZY) ? sinf(now / 90.0f) * 6 : 0;
  float bob = sinf(now / 700.0f) * 2;
  int ex1 = 70 + lookX + wob, ex2 = 170 + lookX + wob;
  int cy = 82 + lookY + bob;

  if (cheek > 0.05f) {                          // yanaklar
    uint16_t cc = mix565(C_BG, C_PINK, cheek * 0.8f);
    int cyk = 136 + lookY * 0.5f;
    cv->fillRoundRect(ex1 - 30, cyk, 32, 14, 7, cc);
    cv->fillRoundRect(ex2 - 2,  cyk, 32, 14, 7, cc);
  }

  drawMouth(120 + lookX * 0.6f + wob, 162 + lookY * 0.5f, now);

  if (state == S_DIZZY) {
    float ph = now / 120.0f;
    drawSpiral(ex1, cy, ph, C_EYE);
    drawSpiral(ex2, cy, -ph, C_EYE);
    for (int i = 0; i < 3; i++) {               // dönen yıldızlar
      float a = now / 260.0f + i * 2.094f;
      cv->fillCircle(120 + cosf(a) * 78, 20 + sinf(a) * 9, 4, C_STAR);
    }
  } else {
    float blinkMul = 1.0f;
    uint32_t bt = now - blinkStart;
    if (bt < 160) blinkMul = 1.0f - sinf(3.14159f * bt / 160.0f);
    float op = eyeOpen * blinkMul;
    float shine = lookX * 0.25f;
    drawEye(ex1, cy, op, happy, shine);
    drawEye(ex2, cy, op, happy, shine);
    if (state == S_SLEEP) drawZzz(now);
  }

  drawSubtitle(now);
}

// ================== SENARYO MOTORU ==================
uint32_t cycleStart = 0;
int stepIdx = -1;

void applyStep(const Step &s, uint32_t now) {
  setState(s.st);
  drowsy = s.drowsy;
  targetLX = s.lx;
  targetLY = s.ly;
  if (s.say) say(s.say, s.hold);
  if (s.happy) happyUntil = now + (s.hold ? s.hold : 1500);
}

void restartCycle(uint32_t now) {
  cycleStart = now;
  stepIdx = -1;
  subN = 0;
  eyeOpen = 0; lookX = lookY = 0; happy = cheek = 0;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  C_BG    = rgb(10, 12, 28);
  C_EYE   = rgb(90, 230, 255);
  C_PINK  = rgb(255, 120, 160);
  C_WHITE = rgb(255, 255, 255);
  C_BAR   = rgb(30, 34, 62);
  C_GRAY  = rgb(170, 180, 200);
  C_STAR  = rgb(255, 220, 80);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, -1);
  tft.init(240, 240, SPI_MODE3);
  tft.setSPISpeed(40000000);   // sorun olursa 27000000 yap
  tft.setRotation(0);
  tft.fillScreen(0);

  cv = new GFXcanvas16(240, 240);
  if (!cv) {
    Serial.println("RAM yok!");
    while (1) delay(1000);
  }

  randomSeed(analogRead(2));
  nextBlink = millis() + 2500;
  restartCycle(millis());
}

// ================== LOOP ==================
void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (now - lastFrame < 33) return;             // ~30 FPS
  lastFrame = now;

  // ---- senaryoyu ilerlet ----
  uint32_t t = now - cycleStart;
  while (stepIdx + 1 < NSTEPS && t >= script[stepIdx + 1].t) {
    stepIdx++;
    applyStep(script[stepIdx], now);
  }
  if (t >= CYCLE_END) restartCycle(now);

  // ---- animasyon hedefleri ----
  float openTarget = 1.0f;
  float rate = 0.25f;
  if (state == S_SLEEP) { openTarget = 0.0f; rate = 0.12f; }
  else if (state == S_WAKING) { rate = 0.07f; }
  else if (drowsy) { openTarget = 0.30f; rate = 0.06f; }
  eyeOpen += (openTarget - eyeOpen) * rate;

  float tx = targetLX, ty = targetLY;
  if (state == S_AWAKE) {                       // hafif canlı bakış
    tx += sinf(now / 1300.0f) * 3;
    ty += sinf(now / 1700.0f) * 2;
  }
  if (state == S_SLEEP || state == S_LOGO) { tx = 0; ty = 0; }
  lookX += (tx - lookX) * 0.12f;
  lookY += (ty - lookY) * 0.12f;

  float hTarget = (now < happyUntil && state != S_DIZZY) ? 1.0f : 0.0f;
  happy += (hTarget - happy) * 0.2f;
  float cTarget = (state == S_DIZZY) ? 0.9f : max(happy, state == S_WAKING ? 0.8f : 0.0f);
  cheek += (cTarget - cheek) * 0.15f;

  if (now > nextBlink) {                        // göz kırpma
    blinkStart = now;
    nextBlink = now + random(2200, 5000);
  }

  // ---- çiz ve ekrana bas ----
  if (state == S_LOGO) drawLogoScreen();
  else drawFace(now);
  tft.drawRGBBitmap(0, 0, cv->getBuffer(), 240, 240);
}
