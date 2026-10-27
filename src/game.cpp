// ===================================================================
//  BRAWL MICROSTARS v4 — Shooter 2D type Brawl Stars pour NumWorks
//  App externe Epsilon (.nwa via nwlink / EADK)
//
//  CONTROLES (V5.3) :
//    Fleches : bouger | OK ou var : attaque vers la direction du perso
//    RETOUR : ulti (super) | SHIFT : gadget | EFFACER : hypercharge
//    HOME : quitter le match | + * / - : achats dans le menu BRAWLERS
//
//  V5 : menu style Brawl Stars, front montants (plus de double-appui),
//       star drop a ouvrir manuellement, quetes persistantes, IA anti-blocage,
//       but Brawl Ball seulement entre les poteaux, choix du brawler avant match.
// ===================================================================

#include <ion.h>
extern "C" {
#include <eadk.h> // eadk.h n'a pas de garde extern "C" : sans ca, le C++ decore
                  // les noms des fonctions EADK et la calculatrice ne les trouve plus
}
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "epsilon_files.h" // sauvegarde dans le systeme de fichiers d'Epsilon (methode NumPlay)

// L'identite NWA (eadk_app_name / eadk_api_level) est dans src/app_id.c :
// un petit fichier C pur compile sans LTO, car le LTO C++ perd
// l'attribut de section .rodata.eadk_* exige par le chargeur d'Epsilon.

#define SW 320
#define SH 240
#define TILE 14
#define MAPW 32
#define MAPH 24

static uint16_t seed = 12345;
static int rnd(int n) { seed = (uint16_t)(seed * 25173 + 13849); return (seed >> 4) % (n > 0 ? n : 1); }

// Rendu DIRECT a l'ecran via EADK, sans frame buffer (economise ~150 Ko de RAM,
// indispensable sur N0110/N0115 ou la RAM des apps externes est limitee).
// Chaque primitive pousse ses lignes au fur et a mesure, comme NumPlay.
static void px(int x, int y, KDColor c) {
  if (x >= 0 && x < SW && y >= 0 && y < SH) Ion::Display::pushRect(KDRect(x, y, 1, 1), &c);
}
static void fRect(int x, int y, int w, int h, KDColor c) {
  if (w < 0) { x += w; w = -w; } if (h < 0) { y += h; h = -h; }
  if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
  if (x + w > SW) w = SW - x; if (y + h > SH) h = SH - y;
  if (w <= 0 || h <= 0) return;
  Ion::Display::pushRectUniform(KDRect(x, y, w, h), c);
}
// ligne de travail : les primitives a motif remplissent rowBuf puis poussent les runs
static KDColor rowBuf[SW];
static bool rowSet[SW];
static void rowClear(int x0, int x1) {
  if (x0 < 0) x0 = 0; if (x1 >= SW) x1 = SW - 1;
  for (int x = x0; x <= x1; x++) rowSet[x] = false;
}
static inline void rowPut(int x, KDColor c) { if (x >= 0 && x < SW) { rowBuf[x] = c; rowSet[x] = true; } }
static void rowFlush(int y, int x0, int x1) {
  if (y < 0 || y >= SH) return;
  int x = x0 < 0 ? 0 : x0, xe = x1 >= SW ? SW - 1 : x1;
  while (x <= xe) {
    if (!rowSet[x]) { x++; continue; }
    int s = x;
    while (x <= xe && rowSet[x]) x++;
    Ion::Display::pushRect(KDRect(s, y, x - s, 1), rowBuf + s);
  }
}
static void fillSpan(int y, int x0, int x1, KDColor c) {
  if (y < 0 || y >= SH || x0 > x1) return;
  if (x0 < 0) x0 = 0; if (x1 >= SW) x1 = SW - 1;
  for (int x = x0; x <= x1; x++) rowBuf[x] = c;
  Ion::Display::pushRect(KDRect(x0, y, x1 - x0 + 1, 1), rowBuf + x0);
}
static void rRect(int x, int y, int w, int h, int r, KDColor c) {
  for (int j = 0; j < h; j++) {
    int ry = y + j;
    rowClear(x, x + w - 1);
    for (int i = 0; i < w; i++) {
      int dx = (i < r) ? (r - i) : ((i >= w - r) ? (i - (w - r - 1)) : 0);
      int dy = (j < r) ? (r - j) : ((j >= h - r) ? (j - (h - r - 1)) : 0);
      if (dx * dx + dy * dy <= r * r) rowPut(x + i, c);
    }
    rowFlush(ry, x, x + w - 1);
  }
}
static void fCirc(int cx, int cy, int r, KDColor c) {
  for (int j = -r; j <= r; j++) {
    int w = (int)sqrtf((float)(r * r - j * j));
    fillSpan(cy + j, cx - w, cx + w, c);
  }
}
static void oCirc(int cx, int cy, int r, KDColor fill, KDColor outline) {
  for (int j = -r - 1; j <= r + 1; j++) {
    int w2 = (int)sqrtf((float)((r + 1) * (r + 1) - j * j));
    rowClear(cx - w2, cx + w2);
    for (int i = -w2; i <= w2; i++) {
      int d2 = i * i + j * j;
      rowPut(cx + i, (d2 <= r * r) ? fill : outline);
    }
    rowFlush(cy + j, cx - w2, cx + w2);
  }
}
static void shCircRGB(int cx, int cy, int r, uint32_t topRGB, uint32_t botRGB, KDColor outline) {
  KDColor top = KDColor::RGB24(topRGB), bot = KDColor::RGB24(botRGB);
  for (int j = -r - 1; j <= r + 1; j++) {
    int w2 = (int)sqrtf((float)((r + 1) * (r + 1) - j * j));
    float t = (r > 0) ? (float)(j + r) / (2.0f * r) : 0.0f;
    if (t < 0) t = 0; if (t > 1) t = 1;
    KDColor rowCol = KDColor::RGB(
      (uint8_t)(top.red() + (bot.red() - top.red()) * t),
      (uint8_t)(top.green() + (bot.green() - top.green()) * t),
      (uint8_t)(top.blue() + (bot.blue() - top.blue()) * t));
    rowClear(cx - w2, cx + w2);
    for (int i = -w2; i <= w2; i++) {
      int d2 = i * i + j * j;
      rowPut(cx + i, (d2 <= r * r) ? rowCol : outline);
    }
    rowFlush(cy + j, cx - w2, cx + w2);
  }
}
static void fRing(int cx, int cy, int r, KDColor c) {
  for (int a = 0; a < 360; a += 3)
    px(cx + (int)(r * cosf(a * 0.01745f)), cy + (int)(r * sinf(a * 0.01745f)), c);
}
static void arc(int cx, int cy, int r, float a0, float a1, int thick, KDColor c) {
  for (int a = (int)(a0 * 180 / 3.14159f); a < (int)(a1 * 180 / 3.14159f); a++)
    for (int t = r - thick; t <= r; t++)
      px(cx + (int)(t * cosf(a * 0.01745f)), cy + (int)(t * sinf(a * 0.01745f)), c);
}
static void shadow(int cx, int cy, int rx, int ry) {
  for (int j = -ry; j <= ry; j++) {
    rowClear(cx - rx, cx + rx);
    for (int i = -rx; i <= rx; i++)
      if ((i * i * 100) / (rx * rx + 1) + (j * j * 100) / (ry * ry + 1) <= 100)
        rowPut(cx + i, KDColor::RGB24(0x0A1608));
    rowFlush(cy + j, cx - rx, cx + rx);
  }
}
static void blit() { /* rendu direct : rien a pousser */ }

// ------------------------- Palette -----------------------------------
#define C_BG     KDColor::RGB24(0x1B2A41)
#define C_PANEL  KDColor::RGB24(0x3B2A63)
#define C_PANEL2 KDColor::RGB24(0x4A3785)
#define C_BTN    KDColor::RGB24(0xFFC928)
#define C_BTND   KDColor::RGB24(0xD9A400)
#define C_WALL   KDColor::RGB24(0xB08050)
#define C_WALLD  KDColor::RGB24(0x7A5432)
#define C_WALLL  KDColor::RGB24(0xD0A070)
#define C_BUSH   KDColor::RGB24(0x3D7A20)
#define C_BUSH2  KDColor::RGB24(0x4F9A2E)
#define C_BLUE   KDColor::RGB24(0x2E9BFF)
#define C_BLUED  KDColor::RGB24(0x1E6FBF)
#define C_RED    KDColor::RGB24(0xFF3B3B)
#define C_REDD   KDColor::RGB24(0xBF2222)
#define C_WHT    KDColor::RGB24(0xFFFFFF)
#define C_BLK    KDColor::RGB24(0x101010)
#define C_YEL    KDColor::RGB24(0xFFC928)
#define C_ORG    KDColor::RGB24(0xFF8327)
#define C_PUR    KDColor::RGB24(0xB44BFF)
#define C_MAG    KDColor::RGB24(0xFF2EA6)
#define C_CYN    KDColor::RGB24(0x35D4E8)
#define C_GRE    KDColor::RGB24(0x8A8A8A)
#define C_GEM    KDColor::RGB24(0x35D4E8)
#define C_GEMD   KDColor::RGB24(0x1B8FA8)
#define C_BALL   KDColor::RGB24(0xF2F2F2)
#define C_POISON KDColor::RGB24(0x7B2FA0)
#define C_POIS2  KDColor::RGB24(0x5A1F78)
#define C_GREEN  KDColor::RGB24(0x3DDC55)
#define C_GOLD   KDColor::RGB24(0xFFD700)
#define C_DGREY  KDColor::RGB24(0x2A2438)

// Fond violet du menu, pousse par bandes rapides (apres la palette : utilise C_PANEL)
static void bgPanel() {
  for (int y = 0; y < SH; y++) {
    int x = 0;
    while (x < SW) {
      int band = (x + y * 2) / 40;
      int xe = (band + 1) * 40 - y * 2 - 1;
      if (xe >= SW) xe = SW - 1;
      fillSpan(y, x, xe, (band % 2) ? C_PANEL : C_PANEL2);
      x = xe + 1;
    }
  }
}

#define RAR_N 6
static const uint32_t RAR_COL[RAR_N]  = {0xB9C4C9,0x36D4E8,0x38E05C,0xB44BFF,0xFF2EA6,0xFFC928};
static const char*  RAR_NAME[RAR_N]   = {"DEPART","RARE","SUPER RARE","EPIC","MYTHIQ","LEGENDE"};

// ------------------------- Themes de carte (1 par mode) --------------
// Chaque mode a son propre decor : couleurs + type de murs + vegetation
struct MapTheme {
  const char *themeName;
  uint32_t gr1, gr2;     // damier sol
  uint32_t grDecor;      // brins d'herbe / decor clair
  uint32_t wall, wallD, wallL;
  uint32_t bush1, bush2;
  uint8_t wallStyle;     // 0 brique, 1 pierre taille, 2 sacs de sable, 3 crates, 4 glace
  uint8_t decoStyle;     // 0 herbe, 1 fleurs, 2 cranes, 3 lignes stade, 4 neige
};
static const MapTheme THEMES[8] = {
  // GEM : mine cristalline
  {"MINE DE GEMMES", 0x4E8A5A,0x447A4E, 0x66B534, 0x8A7A5A,0x5C4E3A,0xB0A070, 0x3D7A20,0x4F9A2E, 1, 1},
  // BALL : stade foot
  {"STADE", 0x4F9026,0x458522, 0x66B534, 0xC9973B,0x8A6622,0xE0B050, 0x3D7A20,0x4F9A2E, 3, 3},
  // SOLO : desert canion
  {"CANION", 0xC9A05A,0xBD9350, 0xE0B870, 0xB07040,0x7A4A28,0xD09060, 0x7A8A3A,0x8A9A48, 2, 2},
  // DUO : foret sombre
  {"FORET SOMBRE", 0x3A6A3E,0x32603A, 0x4F8A4E, 0x6A5A3A,0x4A3E28,0x8A7A50, 0x2A5A18,0x3A6E22, 0, 0},
  // HEIST : banque / ville
  {"BANQUE", 0x707880,0x666E76, 0x8A929A, 0x9A8A6A,0x6A5A44,0xBAA880, 0x3D7A20,0x4F9A2E, 1, 2},
  // BOUNTY : marais
  {"MARAIS", 0x4E7A5E,0x447056, 0x669A76, 0x6A7A5A,0x4A5640,0x8A9A70, 0x2A5A3A,0x3A6E4A, 2, 0},
  // KO : volcan
  {"VOLCAN", 0x6A4A3A,0x5E4234, 0x8A6A50, 0x4A3A3A,0x2E2424,0x6A5A5A, 0x5A3A20,0x6E4A2A, 0, 2},
  // ZONE : toundra glacee
  {"TOUNDRA", 0x9ABAD0,0x8EAE7A,0xB4CEE2, 0x7A9AC0,0x4E6A8A,0xA0C0E0, 0x5A7A4E,0x6E8E60, 4, 4},
};

// ------------------------- Definitions brawlers ----------------------
enum AtkType { ATK_SHOT, ATK_SPREAD, ATK_LOB };
enum SuperType { SUP_BURST, SUP_DASH, SUP_HEAL, SUP_BARRAGE, SUP_MEGASHOT, SUP_WAVE, SUP_SUMMON,
                 SUP_STEALTH, SUP_SHIELD, SUP_PULL, SUP_PUSH, SUP_WHIRL, SUP_MINES, SUP_UPGRADE,
                 SUP_MECH, SUP_LEAP, SUP_HASTE, SUP_WALL };
enum GadType { GAD_HEAL, GAD_SPEED, GAD_AMMO, GAD_BLAST };
enum StarType { STAR_DMG, STAR_HP, STAR_SPEED, STAR_REGEN };

struct BrawlerDef {
  const char *name;
  int baseHP, baseDmg, range, reloadMs;
  float projSpeed;
  int projCount, spreadDeg;
  AtkType atk;
  SuperType sup;
  GadType gad;
  StarType star;
  int rar;
  int unlockTroph;
  uint32_t colTop, colBot;
  uint8_t projShape;
};

static const BrawlerDef BRAWLERS[55] = {
  {"Shelly",   5600, 1280, 170, 1500, 340, 3, 24, ATK_SPREAD, SUP_BURST,    GAD_SPEED, STAR_DMG,    0,     0, 0x8A4BE0, 0x5C2BA0, 0},
  {"Nita",     5400, 1320, 160, 1400, 330, 1,  0, ATK_SHOT,   SUP_SUMMON,   GAD_HEAL,  STAR_HP,    0,     0, 0xC98A3B, 0x8A5C22, 1},
  {"Colt",     4000,  940, 240, 1600, 420, 6,  8, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   0,     0, 0x3B9AC9, 0x22628A, 1},
  {"Bull",     6600, 1750, 100, 1900, 300, 5, 30, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    1,    60, 0xC9973B, 0x8A6622, 0},
  {"Brock",    3800, 1620, 280, 2000, 360, 1,  0, ATK_SHOT,   SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   1,   120, 0xC93B3B, 0x8A2222, 2},
  {"El Primo", 6800, 1400,  90, 1300, 330, 4, 20, ATK_SPREAD, SUP_DASH,     GAD_HEAL,  STAR_HP,    1,   180, 0x3BA05C, 0x22663A, 0},
  {"Barley",   3600, 1160, 240, 1700, 260, 1,  0, ATK_LOB,    SUP_BARRAGE,  GAD_AMMO,  STAR_REGEN, 1,   240, 0x7BC93B, 0x4F8A22, 3},
  {"Poco",     5600, 1100, 200, 1400, 300, 3, 16, ATK_SPREAD, SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 1,   300, 0xB03BE0, 0x7822A0, 1},
  {"Rosa",     6400, 1250, 120, 1500, 340, 3, 12, ATK_SPREAD, SUP_SHIELD,   GAD_SPEED, STAR_HP,    2,   400, 0x7AC96B, 0x4F8A46, 0},
  {"Jessie",   4600, 1180, 210, 1700, 300, 1,  0, ATK_SHOT,   SUP_SUMMON,   GAD_AMMO,  STAR_REGEN, 2,   500, 0xA03BA0, 0x701F70, 2},
  {"Dynamike", 4000, 1330, 230, 1800, 280, 2, 12, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_DMG,   2,   620, 0xE0B03B, 0xA07822, 3},
  {"Tick",     3000, 1120, 260, 1500, 260, 3, 14, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_REGEN, 2,   750, 0x8AC93B, 0x5C8A22, 3},
  {"8-Bit",    5800, 1350, 260, 2000, 330, 1,  0, ATK_SHOT,   SUP_SUMMON,   GAD_AMMO,  STAR_DMG,   2,   880, 0xC9A03B, 0x8A6E22, 1},
  {"Rico",     4200, 1010, 260, 1500, 400, 5,  6, ATK_SHOT,   SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   2,  1000, 0x3BC9B0, 0x228A78, 1},
  {"Darryl",   6300, 1460, 130, 1400, 380, 2, 14, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    2,  1150, 0xC96B3B, 0x8A4822, 0},
  {"Penny",    4600, 1290, 230, 1700, 340, 1,  0, ATK_SHOT,   SUP_SUMMON,   GAD_HEAL,  STAR_REGEN, 2,  1300, 0xE07B3B, 0xA05222, 0},
  {"Carl",     5500, 1250, 190, 1200, 320, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_HP,    3,  1500, 0x7B9BC9, 0x50688A, 3},
  {"Jacky",    6000, 1300, 110, 1500, 320, 4, 22, ATK_SPREAD, SUP_WHIRL,    GAD_BLAST, STAR_HP,    3,  1700, 0xE0B03B, 0xA07822, 0},
  {"Bo",       5000, 1200, 250, 1800, 310, 3,  9, ATK_SHOT,   SUP_MINES,    GAD_SPEED, STAR_REGEN, 3,  1900, 0x3B7AC9, 0x224F8A, 2},
  {"Emz",      4800, 1180, 200, 1600, 280, 2, 18, ATK_SPREAD, SUP_BURST,    GAD_HEAL,  STAR_REGEN, 3,  2100, 0xE03B8A, 0xA02258, 1},
  {"Stu",      3800, 1080, 220, 1300, 420, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_DMG,   3,  2350, 0xC93BA0, 0x8A2270, 2},
  {"Pam",      6200, 1150, 220, 1800, 300, 4, 10, ATK_SPREAD, SUP_SUMMON,   GAD_HEAL,  STAR_REGEN, 3,  2600, 0xB09B3B, 0x7A6A22, 2},
  {"Frank",    7400, 1800, 140, 2400, 260, 1,  0, ATK_SHOT,   SUP_WAVE,     GAD_AMMO,  STAR_HP,    3,  2900, 0x5C7A9B, 0x3B5068, 3},
  {"Bibi",     5900, 1350, 120, 1300, 360, 3, 15, ATK_SPREAD, SUP_DASH,     GAD_SPEED, STAR_HP,    3,  3200, 0xE05C3B, 0xA03E22, 0},
  {"Bea",      3600,  960, 270, 1500, 380, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   3,  3500, 0xE0C93B, 0xA08A22, 2},
  {"Nani",     3400, 1420, 260, 1900, 340, 1,  0, ATK_SHOT,   SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   4,  3900, 0x9B7BE0, 0x6850A0, 2},
  {"Edgar",    4200, 1050,  80, 1100, 380, 2, 10, ATK_SPREAD, SUP_DASH,     GAD_HEAL,  STAR_REGEN, 4,  4300, 0x3B3B50, 0x22223A, 0},
  {"Griff",    4900, 1080, 230, 1500, 360, 3,  8, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   4,  4700, 0xC9C93B, 0x8A8A22, 1},
  {"Gale",     5300, 1240, 200, 1600, 320, 3, 14, ATK_SPREAD, SUP_PUSH,     GAD_SPEED, STAR_HP,    4,  5100, 0x7BC9E0, 0x508A9B, 2},
  {"Colette",  4800, 1300, 210, 1500, 330, 2,  6, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   4,  5500, 0xE05C9B, 0xA03E68, 1},
  {"Belle",    3900, 1150, 260, 1700, 350, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_SPEED, STAR_DMG,   4,  6000, 0x7B3BE0, 0x5022A0, 2},
  {"Buzz",     6100, 1380, 110, 1300, 370, 3, 18, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    4,  6500, 0x3BE0C9, 0x229B8A, 0},
  {"Ash",      6500, 1290, 180, 1700, 300, 4, 12, ATK_SHOT,   SUP_SUMMON,   GAD_AMMO,  STAR_HP,    4,  7000, 0x9BA05C, 0x686A3A, 3},
  {"Lola",     4400, 1220, 240, 1600, 340, 1,  0, ATK_SHOT,   SUP_SUMMON,   GAD_AMMO,  STAR_DMG,   4,  7500, 0xE03B5C, 0xA0223E, 1},
  {"Fang",     5700, 1420, 140, 1400, 350, 2, 12, ATK_SPREAD, SUP_DASH,     GAD_SPEED, STAR_HP,    4,  8000, 0xC95C3B, 0x8A3E22, 0},
  {"Maisie",   4700, 1260, 240, 1700, 330, 1,  0, ATK_SHOT,   SUP_BURST,    GAD_BLAST, STAR_DMG,   5,  8600, 0x3BE0A0, 0x229B68, 2},
  {"Squeak",   3500, 1200, 250, 1800, 280, 1,  0, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_REGEN, 5,  9200, 0xC93BE0, 0x8A22A0, 3},
  {"Hank",     6900, 1500, 220, 2000, 290, 3, 10, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_HP,    5,  9800, 0x3B6BE0, 0x2247A0, 2},
  {"Pearl",    5200, 1230, 210, 1500, 320, 2,  8, ATK_SPREAD, SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 5, 10500, 0xE09B3B, 0xA06822, 1},
  {"Spike",    3200, 1120, 250, 1900, 300, 3, 20, ATK_LOB,    SUP_BURST,    GAD_SPEED, STAR_DMG,   5, 11200, 0x7BE05C, 0x4FA03E, 3},
  {"Crow",     3200, 1080, 260, 1500, 400, 3,  6, ATK_SHOT,   SUP_BARRAGE,  GAD_SPEED, STAR_DMG,   5, 12000, 0x5C3BE0, 0x3E22A0, 2},
  {"Leon",     4200, 1160, 220, 1300, 390, 4,  7, ATK_SPREAD, SUP_STEALTH,  GAD_SPEED, STAR_DMG,   5, 12800, 0x3BE07B, 0x22A050, 1},
  {"Sandy",    4600, 1180, 240, 1600, 320, 3,  9, ATK_LOB,    SUP_STEALTH,  GAD_AMMO,  STAR_REGEN, 5, 13700, 0x7B7BE0, 0x5050A0, 2},
  {"Amber",    4800, 1090, 200, 1200, 340, 2,  4, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   5, 14600, 0xE0C95C, 0xA08A3E, 1},
  {"Gene",     4500, 1230, 250, 1700, 310, 1,  0, ATK_SHOT,   SUP_PULL,     GAD_HEAL,  STAR_REGEN, 5, 15500, 0x9B3BE0, 0x6822A0, 2},
  {"Mortis",   5000, 1260, 150, 1300, 380, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_HP,    5, 16500, 0x3B3B5C, 0x22223E, 3},
  {"Byron",    3900, 1050, 270, 1800, 340, 1,  0, ATK_SHOT,   SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 5, 17500, 0xC9C9C9, 0x8A8A8A, 2},
  {"Meg",      4100, 1150, 230, 1500, 330, 2,  9, ATK_SHOT,   SUP_MECH,     GAD_AMMO,  STAR_DMG,   5, 18600, 0xE03BC9, 0xA0228A, 1},
  {"Chester",  4600, 1210, 210, 1400, 350, 3, 11, ATK_SPREAD, SUP_BARRAGE,  GAD_BLAST, STAR_DMG,   5, 19800, 0x5CC9E0, 0x3E8A9B, 0},
  {"Surge",    4500, 1250, 200, 1400, 360, 3, 13, ATK_SPREAD, SUP_UPGRADE,  GAD_SPEED, STAR_DMG,   5, 21000, 0x3BE0E0, 0x229B9B, 2},
  {"Piper",    3200, 1780, 340, 2000, 430, 1,  0, ATK_SHOT,   SUP_LEAP,     GAD_AMMO,  STAR_DMG,   3, 22000, 0xE05C9B, 0xA03E68, 2},
  {"Max",      4200, 1000, 200, 1100, 400, 4,  6, ATK_SPREAD, SUP_HASTE,    GAD_SPEED, STAR_SPEED, 4, 23000, 0xE03B7B, 0xA02250, 2},
  {"Tara",     4400, 1150, 240, 1700, 330, 3,  5, ATK_SHOT,   SUP_PULL,     GAD_HEAL,  STAR_DMG,   5, 24000, 0x8A2BE0, 0x5C1DA0, 2},
  {"Sprout",   3600, 1100, 250, 1800, 290, 1,  0, ATK_LOB,    SUP_WALL,     GAD_BLAST, STAR_REGEN, 5, 25000, 0x7BE05C, 0x4FA03E, 3},
  {"Mr. P",    4600, 1100, 240, 1700, 300, 1,  0, ATK_LOB,    SUP_SUMMON,   GAD_AMMO,  STAR_REGEN, 4, 26000, 0x3B7AC9, 0x224F8A, 3},
};

// ------------- Traits visuels uniques par brawler --------------------
// Derives deterministes de l'index : chaque brawler a sa coiffure,
// son chapeau/accessoire et sa couleur d'accent. 50 looks differents.
// Noms des gadgets et pouvoirs star pour chaque brawler (style BS).
static const char *GADGET_NAMES[55] = {
  "BANDANA VELOCE","OURSON SURPRISE","BALLES PERCUTANTES","MURAILLE","ROQUETTES TANDEM",
  "SUPLEX AERIEN","TISSU REPARATEUR","JINGLE TRISTE","GANTS DURCIS","SCRAPPIES FURIEUX",
  "SAUTECARABINES","MINE ANTI-CHAR","TETE HURLANTE","PING BALLE AVIDE","BARIL DOUBLE",
  "SAC DE POUDRE","ROUE SECOURS","SECOUSSES","OISEAU ECLAIRE","SPRAY EMOUVANT",
  "VROUM TURBO","SOIN INTENSIF","ETONNEMENT","BONBON GOMME","RAYON AFFAME",
  "PEUGEOT PROTEGE","VOL PORTE-CLE","PORTAIL TORDU","COFFRE PIEGE","RAFALE GLACEE",
  "FACTURE IMPOSEE","BALLE CHAINOT","BOUCLIER BOB","POUBELLE PLEINE","LILAS CHERO",
  "COUP SAVATE","FRAPPE ARDENTE","PUSTULE ADHESIVE","GONFLETTE","PERLE DOUCE",
  "TERRAIN SABLEUX","TOXINE RALENTIE","FUMEE FANTOME","SABLE TROUBLE","HUILE ECLAIRANTE",
  "LAMPADAIRE","MELANGE CAFE","BOUSSOLE OR","BLAST RONFANT","SURGE ALTERNATIF",
  "PARACHUTE PERCUT","TABLETTE ENERGIE","TAROT PIEGE","GRAINES DURCIES","VALISE COURSE"
};
static const char *STAR_NAMES[55] = {
  "COQUILLE DURE","AMITIE FEROCE","TIR VELOCE","PEAUX DURES","ROQUETTES LONGUES",
  "MUAY PRIMO","SIROP EPAIS","GUERISON DAIRY","GANTS FERTILES","SURCHARGE",
  "DYNAMITE GRANDE","CARAPACE","SAUT LONG","ROULE-BALLON","SAC MAGIQUE",
  "POT DE POUDRE","CHALEUR FROIDE","SEISME","TROISIEME OEIL","CŒUR TENDRE",
  "VITESSE ECLAIR","SOIN MATERNEL","FORCE HERCULE","RACQUETTE","RAYON GEL",
  "PEUGEOT RAPIDE","GRIFFES LESTES","REMISE RONDE","COFFRE OUVERT","GIVRE EPINEUX",
  "ECONOMIE","GRAND ECLAIR","BOUCLIER MAITRE","POUBELLE FUMANTE","LILAS DORE",
  "SAVATE FORT","BRASIER","PUSTULE","GONFLE","DOUCE PERLE",
  "SABLE SLOW","VENIN","FANTOME","TROUBLE","HUILE",
  "LAMPE","CAFE SERRE","BOUSSOLE","RONFLEMENT","ALTERNANCE",
  "MUNITION FUMANTE","DEMARRE RAPIDE","DESTIN TRACE","PHOTOSYNTHESE","POURBOIRE GENEREUX"
};
static const char *BRAWLER_BIOS[55] = {
  "FUSIL A POMPE DEPANNEUSE","AMIE DES OURS","DEUX PISTOLETS STAR","BRUTE DE COWBOY","FUSIL ROQUETTES",
  "LUTTEUR MASQUE","BARMAN ROBOT","MARIACHI DOUX","BOTANISTE BOXEUSE","INGENIEURE SCRAPPY",
  "DEMINEUR FOU","TETE A CLAQUES","ARCADE AMBULANTE","RICOCHET MAITRE","TONNEAU ROULANT",
  "PIRATE AU SABLE","MINEUR FOU","OUVRIERE DE CHANTIER","ARCHER SIOUX","INFLUENCEUSE TOXIQUE",
  "PILOTE DE COURSE","MAMAN SOIGNEUSE","MONSTRE FRANKENSTEIN","BATTEUSE DE BASEBALL","ABEILLE REINE",
  "GRAND-MERE A DISTANCE","CAPEUR DES TOITS","PATRON AVARE","MAITRE DU VENT","FACTRICE IMPIOYE",
  "VOLUSE MASQUEE","SAUVEUR DE PLAGE","CENDRILLON COLERE","SPECTRE ECRIVAIN","DIVA DE SCENE",
  "MAITRE DU KUNG-FU","FOCUS BRULANT","ALIEN ELASTIQUE","GENERAL POISSON","PATISSIERE FEROCE",
  "CACTUS LEGENDAIRE","CORBEAU ASSASSIN","NINJA INVISIBLE","SPECTRE SOMNOLANT","ALCHIMISTE FEERIE",
  "GENIE DE LAMPE","VAMPIR MOUSTACHE","ROBOT PILOTE","CLOWN IMPREVISIBLE","SURGE ELECTRIQUE",
  "SNIPER DE FETE","COACH SPORTIVE","CARTOMANCIENT","JARDINIER VOLANT","PORTEUR TROP POLI"
};
// Noms et descriptions des SUPERS (inspires de Brawl Stars)
static const char *SUPER_NAMES[55] = {
  "SUPER COQUILLE","ATTENTION OURS","BALLE DE DESTRUCTION","CHARGE TAURAU","PLUIE DE ROQUETTES",
  "METEOR PRIMO","ULTIME COCKTAIL","GUERISON ENCOREE","CARAPACE VEGETALE","SCRAPPY FURIEUX",
  "GRANDE DYNAMITE","MINE REJETEE","TOURELLE BOOSTEUSE","PLUIE RICOCHET","DOUBLE BARIL",
  "MORTIER","FURIE CARROUEL","SEISME TOTAL","TRIPLE MINE","SPRAY MASSIF",
  "TURBO BURNOUT","STATION DE SOIN","MASSUE SEISME","BALLE FURIEUSE","ESSAIM GEANT",
  "PEEP AUTO","SAUT OMBREUX","PLUIE DE PIECES","BLIZZARD POLAIRE","PERCUTE TOUT",
  "MARQUE POSEE","TORPILLE SAISIE","RATS EN COLERE","EGO SURSCENE","SAVATE VOLANTE",
  "CHOC ARDENT","BLOB SPLITTEUR","BALLON MASTODONTE","FOURNAISE","ZONE EPINEUSE",
  "CRAQUE-CORBEAU","FUMEE FANTOME","SABLE TEMPETE","HUILE BRULANTE","MAIN MAGIQUE",
  "BAIN DE SANG","SERUM VITAL","MEGA ROBOTS","CLOCHE FURIEUSE","SURGE SUPREME",
  "ESCAPE FUMANT","PARTANCE ECLAIR","TROU NOIR","MUR NATUREL","PORTAGE RAPIDE"
};
static const char *SUPER_DESC[55] = {
  "RAFALE LARGE ET DEMOLITION","INVOQUE UN OURSON FURIEUX","PLUIE DE BALLES PERCUTANTES",
  "CHARGE QUI EJECTE LES MURS","CARPETTE DE ROQUETTES","SAUT QUI ECRASE",
  "PLUIE DE BOUTEILLES","SOIGNE TOUTE L EQUIPE","CARAPACE MOINS DE DEGATS",
  "INVOQUE LA TOURELLE SCRAPPY","BARRAGE DE DYNAMITE","GROSSE MINE TUEUSE",
  "INVOQUE UNE TOURELLE","BALLES REBONDISSANTES","ROULE ET DEGONFLE",
  "INVOQUE UN CANON","CHARGE TOURNOYANTE","ASPIRE ET FRAPPE AUTOUR",
  "POSE TROIS MINES","SPRAY QUI RALENTIT","DASH VELOCE",
  "STATION QUI SOIGNE","ONDE SEISMIQUE PERCENTE","CHARGE FRAPPE RECUL",
  "ESSAIM PERCENT","BOMBE GUIDEE PEPP","SAUT SUR L ENNEMI",
  "PLUIE DE PIECES","BLIZZARD QUI REPOUSSE","CHARGE PERFIDE",
  "TIR MARQUEUR","SAISIT UN ENNEMI","INVOQUE DEUX RATS",
  "EGO QUI TIRE AVEC VOUS","SAVATE QUI FRAPPE","TIR QUI REPOUSSE",
  "BLOB QUI SE DIVISE","BALLON GIGANTESQUE","FLAMME EN ZONE",
  "CHAMP RALENTI","DASH DE DAGUES","INVISIBILITE TOTALE",
  "TEMPETE CACHE L EQUIPE","FLAQUE DE FEU","MAIN QUI ASPIRE",
  "CHAUVES-SOURIS VAMPIRES","SOIN ET POISON","TRANSFORME EN MECHA",
  "SURPRISE EXPLOSIVE","PALIER PLUS DE BALLES","BOND ARRIERE GRENADES",
  "VITESSE D EQUIPE","ASPIRE TOUT AUTOUR","POUSSE UN MUR",
  "PINGOUIN PORTEUR"
};
// Gadget n2 pour chaque brawler (choix entre les deux, comme dans BS)
static const char *GADGET2_NAMES[55] = {
  "REVOLVER PESANT","REGLAGE MECANIQUE","ARGENT MAGIQUE","TASSE CAFE","DEUX ROQUETTES",
  "METEOR RETOUR","BARIL ROULANT","HARMONICA","CROISSIERE","SURCHARGE SUPREME",
  "BARRIL CHARGE","CARAPACE RAPEE","ARCADE BONUS","MUNITIONS DOREES","TONNEAU RAPIDE",
  "MORTIER COURT","TONNERRE CHIRURGICAL","TROU NOIR","TIR PERCENT","RESEAU SOCIAL",
  "COURSE BOOSTEE","MUNITIONS INFINIES","FRAPPE D ECLAIR","BALLE RAPIDE","ESSAIM PROTEGE",
  "DISTRACTION","PIEGE OURLI","GRIFFES RETOUR","RAFALE LONGUE","GELEE SECHE",
  "TIR TRANSVERBE","BALLE SOLIDAIRE","FRIGO FORT","RAT EN COLERE","BOUQUET FINAL",
  "MARTEAU SEC","BRASIER PROTEGE","PUSTULE GONFLEE","GAGNE UN CHAT","PERLE CHAUDE",
  "TIR D EPEINE","PLUME MAGIQUE","SILENCE TOTALE","SABLE COLLANT","FLACON RAPIDE",
  "VŒU PROTEGE","GRAND SOIN","REPARATION LONGUE","CLOCHE PIRATE","SURGE GELE",
  "PERLE PARACHUTE","ENERGIE PARTAGEE","TAROT TIRE","GRAINE FORTE","BAGAGE PERDU"
};
// Pouvoir star n2 pour chaque brawler
static const char *STAR2_NAMES[55] = {
  "BANDE PLAISANTE","BONNE ETOILE","DEUXIEME CHARGE","PEAUX TRES DURES","ROQUETTE DOUBLE",
  "PRIMO PRIME","BOISSON LONGUE","CHANSON DOUCE","ROSA ROBUSTE","PANNEAU SOLIDE",
  "SAUT DYNAMITE","TETE MAGNETIQUE","ARCADE PUISSANTE","RIC-ORIGINE","ROULE DOUBLE",
  "PIERRE POKER","FRAPPE PROFONDE","OUVRIERE CHERE","OEIL D AIGLE","CŒUR FORT",
  "VROUM MAXI","MERE TOUTE PUISSANTE","ETONNEMENT SUPREME","COURSES RAPIDES","DARD PERCENT",
  "PEUGEOT TURBO","TIR GRELE","PORTAIL MULTIPLE","CYCLONE RAPIDE","GLACE PROFONDE",
  "REMISE DOUBLE","SAUVETAGE","POUBELLE TRIEE","ASHER ANTIQUE","LILAS MAJEUR",
  "SAVATE LONGUE","BRISE-OS","PUSTULES DOUBLE","POISSON ADULTE","PERLE MAJEURE",
  "CHAMPS PROTEGE","CORBEAU CHER","PAS RAPIDE","SOMBRE PERLE","HUILE MAJEURE",
  "LAMPE MAGIQUE","SOIN FINAL","MEGA PILOTAGE","CLOCHE DOUBLE","SURGE MAJEUR",
  "PORTEE ROSEE","CARDIO PINK","MAIN TELLURIQUE","PELLE SOLLIDE","PANIER LARGE"
};
// Descriptions des gadgets (une par brawler)
static const char *GADGET_DESC[55] = {
  "VITESSE +30 PENDANT 3S","SOIGNE 30 POURCENT","TIR +25 POURCENT","GAGNE UN BOUCLIER",
  "RECHARGE INSTANTANEE","SOIGNE PAR SAUT","SOIGNE LES ALLIES","SOIGNE EN ZONE",
  "REDUIT DEGATS SUBIS","REPARATION COMPLETE","SAUTE LES MURS","EXPLOSE EN ZONE",
  "PETRIFIE UN ENNEMI","RECHARGE LES MUNITIONS","DEGAGE LES ENNEMIS","CAISSE DE POUDRE",
  "INVINCIBLE 1S","REPOUSSE TOUS","REVEILLE UN ALLIE","RALENTIT EN ZONE",
  "BOOST IMMEDIAT","SOIGNE EN ZONE","FRAPPE TOUS AUTOUR","BOOST VITESSE","PROJECTILE GEANT",
  "PROTECTION TOTALE","SAUT LONGUE PORTEE","TELEPORTE","RAFALE DE VENT","GELE LES ENNEMIS",
  "PIEGE EXPLOSIF","SAISIT UN ENNEMI","REPOUSSE AVEUGLE","INVOCHE UN RAT","FLEURS SOIGNANTES",
  "REPARE UN MUR","BLESSE EN ZONE","COLLE UN ENNEMI","GEANT BOUCLIER","SURGE D EGOUT",
  "INVOCHE UN CACTUS","RALENTIT LES TIRS","PAS DE TRACES","MAL DE SABLE","GLISSADE LONGUE",
  "SOIN GROUPE","MARQUE LA CIBLE","REPARATION TOTALE","SURPRISE EXPLOSIVE","DECHARGE TOTALE",
  "RECHARGE DU SNIPER","COURT VITE","PREVOIT L AVENIR","POUSSE MUR","TIP DE BAGAGE"
};
struct Look {
  uint8_t hair;      // 0..5 : ras, epique, long, oiseau/ninja-faux, champignon, crete
  uint8_t hat;       // 0..7 : aucun, casquette, chapeau melon, casque mineur,
                     //        turban, toque, casquette arriere, couronne
  uint32_t accent;   // couleur d'accent (cheveux/chapeau)
  uint8_t eyes;      // 0..2 : ronds blancs, fendus, grands
  uint8_t size;      // 0..2 : petit, normal, grand
  uint8_t weapon;    // 0..4 : mains, fusil, lance, hache, baton
};
static void brawlerLook(int i, Look &L) {
  uint32_t acc[] = {0x3B2A63,0x8A2A2A,0x2A5A8A,0x5A8A2A,0x8A5A2A,0x2A8A7A,0x8A2A6A,0x5A5A8A};
  L.hair = (i * 7 + 3) % 6;
  L.hat  = (i * 5 + 1) % 8;
  L.accent = acc[i % 8];
  L.eyes = (i * 3) % 3;
  L.size = (i * 11 + 2) % 3;
  L.weapon = (i * 13) % 5;
}

// dessine le sprite complet d'un brawler (corps + traits uniques)
// walk : phase d'animation de marche (0 = immobile)
static void drawBrawlerSprite(int cx, int cy, int r, int bIdx, int faceAng, bool flash, int walk = 0, int attackT = 0) {
  const BrawlerDef &d = BRAWLERS[bIdx];
  Look L; brawlerLook(bIdx, L);
  // recul du corps pendant l'attaque (petit kick oppose a la visee)
  if (attackT > 0) {
    int kick = (attackT > 120) ? 2 : 1;
    cx -= (int)(cosf(faceAng * 0.01745f) * kick);
    cy -= (int)(sinf(faceAng * 0.01745f) * kick);
  }
  int rr = r + (L.size == 2 ? 2 : (L.size == 0 ? -1 : 0));
  KDColor acc = KDColor::RGB24(L.accent);
  // cape derriere les mythiques et legendaires
  if (d.rar >= 4 && r >= 6) {
    int flap = (walk / 60) % 2;
    for (int j = -rr; j < rr; j += 2)
      fRect(cx - rr - 3 + flap, cy + j, 2, 2, acc);
  }
  // jambes animees (petits rectangles alternes sous le corps)
  if (walk > 0 && r >= 6) {
    int step = (walk / 40) % 2;
    KDColor legC = KDColor::RGB24(d.colBot);
    fRect(cx - 3, cy + rr - 1, 3, 3, legC);
    fRect(cx + 1, cy + rr - 1 - (step ? 1 : 0), 3, 3, legC);
    px(cx - 3, cy + rr + 2, C_BLK); px(cx + 1, cy + rr + 1 - (step ? 1 : 0), C_BLK);
    px(cx - 1, cy + rr + 2, C_BLK); px(cx + 3, cy + rr + 1 - (step ? 1 : 0), C_BLK);
  }
  // corps degrade
  shCircRGB(cx, cy, rr, flash ? 0xFFFFFF : d.colTop, flash ? 0x9A9A9A : d.colBot, C_BLK);
  // bras oriente vers la visee
  int gx = (int)(cosf(faceAng * 0.01745f) * (rr + 3)), gy = (int)(sinf(faceAng * 0.01745f) * (rr + 3));
  KDColor wc = KDColor::RGB24(0x6A4A2A);
  if (r >= 6) fRect(cx + gx / 2 - 1, cy + gy / 2 - 1, 3, 3, KDColor::RGB24(d.colBot));
  // arme selon type
  if (L.weapon == 1) { fRect(cx + gx - 2, cy + gy - 2, 6, 2, wc); fRect(cx + gx + 3, cy + gy - 1, 2, 3, C_GRE); }
  else if (L.weapon == 2) { fRect(cx + gx - 1, cy + gy - 3, 2, 7, wc); fCirc(cx + gx, cy + gy - 4, 2, C_GRE); }
  else if (L.weapon == 3) { fRect(cx + gx - 1, cy + gy - 3, 2, 7, wc); fRect(cx + gx - 3, cy + gy - 4, 6, 2, C_GRE); }
  else if (L.weapon == 4) { fRect(cx + gx - 1, cy + gy - 3, 2, 8, wc); fCirc(cx + gx, cy + gy + 4, 2, acc); }
  // yeux
  int ex = (int)(cosf(faceAng * 0.01745f) * 2), ey = (int)(sinf(faceAng * 0.01745f) * 2);
  if (L.eyes == 0) { fRect(cx + ex - 3, cy + ey - 2, 2, 3, C_WHT); fRect(cx + ex + 1, cy + ey - 2, 2, 3, C_WHT); }
  else if (L.eyes == 1) { fRect(cx + ex - 3, cy + ey - 1, 2, 1, C_WHT); fRect(cx + ex + 1, cy + ey - 1, 2, 1, C_WHT); }
  else { fRect(cx + ex - 4, cy + ey - 3, 3, 4, C_WHT); fRect(cx + ex + 1, cy + ey - 3, 3, 4, C_WHT); }
  // bouche (sourire cartoon sur les grands sprites)
  if (r >= 8) fRect(cx + ex - 2, cy + ey + 3, 5, 1, C_BLK);
  // coiffure
  switch (L.hair) {
    case 1: fCirc(cx, cy - rr + 1, 3, acc); fCirc(cx - 2, cy - rr, 2, acc); break;   // mohawk
    case 2: fRect(cx - rr, cy - 2, 3, rr + 1, acc); fRect(cx + rr - 3, cy - 2, 3, rr + 1, acc); break; // longue
    case 3: fCirc(cx + rr - 1, cy - rr + 1, 2, acc); break;  // petit oiseau
    case 4: fCirc(cx, cy - rr + 2, 4, acc); px(cx - 2, cy - rr, C_WHT); px(cx + 2, cy - rr, C_WHT); break; // champi
    case 5: for (int s = -1; s <= 1; s++) fRect(cx + s * 3 - 1, cy - rr - 1, 2, 3, acc); break; // crete
    default: break;
  }
  // chapeau par-dessus
  switch (L.hat) {
    case 1: fRect(cx - 4, cy - rr - 2, 8, 3, acc); fRect(cx + 2, cy - rr, 4, 2, acc); break; // casquette
    case 2: fRect(cx - 5, cy - rr - 3, 10, 4, acc); fRect(cx - 7, cy - rr, 14, 2, acc); break; // melon
    case 3: fRect(cx - 5, cy - rr - 2, 10, 3, C_YEL); fCirc(cx, cy - rr - 4, 2, C_WHT); break; // casque
    case 4: fCirc(cx, cy - rr - 1, 4, acc); fRect(cx - 2, cy - rr + 2, 5, 2, C_WHT); break; // turban
    case 5: fRect(cx - 5, cy - rr - 4, 10, 4, C_WHT); fRect(cx - 6, cy - rr, 12, 2, C_WHT); break; // toque
    case 6: fRect(cx - 5, cy - rr - 2, 8, 3, acc); break; // casquette arriere
    case 7: fRect(cx - 5, cy - rr - 3, 10, 3, C_GOLD); px(cx - 3, cy - rr - 5, C_MAG); px(cx, cy - rr - 6, C_MAG); px(cx + 3, cy - rr - 5, C_MAG); break; // couronne
    default: break;
  }
  // flash de bouche au bout de l'arme pendant l'attaque
  if (attackT > 0 && r >= 4) {
    int fx = (int)(cosf(faceAng * 0.01745f) * (rr + 6)), fy = (int)(sinf(faceAng * 0.01745f) * (rr + 6));
    fCirc(cx + fx, cy + fy, attackT > 120 ? 3 : 2, attackT > 120 ? C_WHT : C_YEL);
    fCirc(cx + fx, cy + fy, 1, C_ORG);
    px(cx + fx + 1, cy + fy - 1, C_WHT); px(cx + fx - 1, cy + fy + 1, C_WHT);
  }
}

// ------------------------- Compte / progression ----------------------
struct BrawlerAccount { int level, trophies; bool gadget, starPower, hyper; uint8_t gadChoice, starChoice; };
struct Account { BrawlerAccount b[55]; int coins; } acc;
static int unlockedBrawlers = 0;

// gadget/star equipe selon le choix du joueur
static const char* curGadgetName(int i) {
  return acc.b[i].gadChoice == 1 ? GADGET2_NAMES[i] : GADGET_NAMES[i];
}
static const char* curStarName(int i) {
  return acc.b[i].starChoice == 1 ? STAR2_NAMES[i] : STAR_NAMES[i];
}

// ---- Sauvegarde persistante (systeme de fichiers interne d'Epsilon) ----
// Methode NumPlay : ecriture directe des records via epsilon_files.h.
// Format : 55*6 (niveau, trophees, flags, choix gadget/star) + pieces + quetes + compteurs = 361 octets.
struct Quest { int type; int goal; int prog; int reward; };
static Quest quests[3];
static int killCounter = 0, gemCounter = 0, superCounter = 0, winStreak = 0;
static const char SAVE_NAME[] = "minibs.sav";
static void saveAcc() {
  uint8_t buf[366]; // 55*6 + 2 pieces + 21 quetes + 8 compteurs = 361 octets
  int p = 0;
  for (int i = 0; i < 55; i++) {
    buf[p++] = (uint8_t)acc.b[i].level;
    buf[p++] = (uint8_t)(acc.b[i].trophies >> 8);
    buf[p++] = (uint8_t)(acc.b[i].trophies & 0xFF);
    buf[p++] = (uint8_t)((acc.b[i].gadget ? 1 : 0) | (acc.b[i].starPower ? 2 : 0) | (acc.b[i].hyper ? 4 : 0));
    buf[p++] = acc.b[i].gadChoice;
    buf[p++] = acc.b[i].starChoice;
  }
  buf[p++] = (uint8_t)(acc.coins >> 8);
  buf[p++] = (uint8_t)(acc.coins & 0xFF);
  for (int i = 0; i < 3; i++) {
    buf[p++] = (uint8_t)quests[i].type;
    buf[p++] = (uint8_t)(quests[i].goal >> 8);
    buf[p++] = (uint8_t)(quests[i].goal & 0xFF);
    buf[p++] = (uint8_t)(quests[i].prog >> 8);
    buf[p++] = (uint8_t)(quests[i].prog & 0xFF);
    buf[p++] = (uint8_t)(quests[i].reward >> 8);
    buf[p++] = (uint8_t)(quests[i].reward & 0xFF);
  }
  buf[p++] = (uint8_t)(killCounter >> 8); buf[p++] = (uint8_t)(killCounter & 0xFF);
  buf[p++] = (uint8_t)(gemCounter >> 8);  buf[p++] = (uint8_t)(gemCounter & 0xFF);
  buf[p++] = (uint8_t)(superCounter >> 8);buf[p++] = (uint8_t)(superCounter & 0xFF);
  buf[p++] = (uint8_t)(winStreak >> 8);   buf[p++] = (uint8_t)(winStreak & 0xFF);
  ef_write(SAVE_NAME, buf, (uint32_t)p);
}
static bool loadAcc() {
  uint32_t len = 0;
  const uint8_t *buf = ef_read(SAVE_NAME, &len);
  if (!buf || (len != 231 && len != 331 && len != 361)) return false; // format inattendu : partie vierge
  int nb = (len == 361) ? 55 : 50; // nombre de brawlers presents dans la sauvegarde
  int p = 0;
  for (int i = 0; i < 55; i++) {
    if (i >= nb) { // brawlers ajoutes apres coup : compte vierge
      acc.b[i].level = 0; acc.b[i].trophies = 0;
      acc.b[i].gadget = acc.b[i].starPower = acc.b[i].hyper = false;
      acc.b[i].gadChoice = acc.b[i].starChoice = 0;
      continue;
    }
    acc.b[i].level = buf[p++];
    int t = (buf[p] << 8) | buf[p + 1]; p += 2;
    acc.b[i].trophies = t;
    uint8_t flags = buf[p++];
    acc.b[i].gadget = (flags & 1) != 0;
    acc.b[i].starPower = (flags & 2) != 0;
    acc.b[i].hyper = (flags & 4) != 0;
    if (len != 231) {
      acc.b[i].gadChoice = buf[p++] & 1;
      acc.b[i].starChoice = buf[p++] & 1;
    } else { acc.b[i].gadChoice = 0; acc.b[i].starChoice = 0; }
    if (acc.b[i].level > 11) acc.b[i].level = 11;
    if (acc.b[i].trophies < 0 || acc.b[i].trophies > 9999) acc.b[i].trophies = 0;
  }
  acc.coins = (buf[p] << 8) | buf[p + 1];
  if (acc.coins < 0) acc.coins = 0;
  p += 2;
  for (int i = 0; i < 3; i++) {
    quests[i].type = buf[p++];
    quests[i].goal = (buf[p] << 8) | buf[p + 1]; p += 2;
    quests[i].prog = (buf[p] << 8) | buf[p + 1]; p += 2;
    quests[i].reward = (buf[p] << 8) | buf[p + 1]; p += 2;
    if (quests[i].type > 4) { quests[i].type = 0; quests[i].goal = 1; quests[i].prog = 0; quests[i].reward = 80; }
  }
  killCounter = (buf[p] << 8) | buf[p + 1]; p += 2;
  gemCounter  = (buf[p] << 8) | buf[p + 1]; p += 2;
  superCounter = (buf[p] << 8) | buf[p + 1]; p += 2;
  winStreak   = (buf[p] << 8) | buf[p + 1];
  if (winStreak < 0) winStreak = 0;
  return true;
}

static int upgradeCost(int lvl) { return 20 + lvl * 30; }
static float lvlMul(int lvl) { return 1.0f + (lvl - 1) * 0.05f; }

static int totalTrophies() {
  int t = 0;
  for (int i = 0; i < 55; i++) t += acc.b[i].trophies;
  return t;
}
static bool isUnlocked(int i) { return acc.b[i].level > 0; }

static void refreshUnlocks(int *newlyUnlocked) {
  *newlyUnlocked = -1;
  int tt = totalTrophies();
  for (int i = 0; i < 55; i++)
    if (acc.b[i].level == 0 && tt >= BRAWLERS[i].unlockTroph) {
      acc.b[i].level = 1; acc.b[i].trophies = 0;
      acc.b[i].gadget = false; acc.b[i].starPower = false; acc.b[i].hyper = false;
      *newlyUnlocked = i;
    }
  unlockedBrawlers = 0;
  for (int i = 0; i < 55; i++) if (isUnlocked(i)) unlockedBrawlers++;
}

static void initQuests(); // prototype : definie plus bas, appelee par initAcc
static void initAcc() {
  memset(&acc, 0, sizeof(acc));
  acc.coins = 100;
  if (loadAcc()) {
    // compte restaure : on recalcule juste les debloquages derives
    int newUnlock = -1;
    refreshUnlocks(&newUnlock);
    saveAcc();
    return;
  }
  // partie vierge : quetes initiales puis sauvegarde complete
  initQuests();
  int newUnlock = -1;
  refreshUnlocks(&newUnlock);
  saveAcc();
}

// ------------------------- Modes --------------------------------------
enum GameMode { MODE_GEM = 0, MODE_BALL, MODE_SOLO, MODE_DUO, MODE_HEIST, MODE_BOUNTY, MODE_KO, MODE_ZONE, MODE_COUNT };
static const char *MODE_NAMES[MODE_COUNT] = {
  "RAMASSE-GEMMES 3V3", "BRAWL BALL 3V3", "SURVIVANT 6J", "SURVIVANT DUO 4X2",
  "BRAQUAGE 3V3", "PRIME 3V3", "A-MORT 3V3", "ZONE CHAUDE 3V3"
};
static const char *MODE_DESC[MODE_COUNT] = {
  "10 GEMMES + 15S", "2 BUTS POUR GAGNER", "DERNIER EN VIE", "DERNIERE EQUIPE",
  "DETROUISE LE COFFRE", "20 ETOILES DE KILLS", "ELIMINE L'EQUIPE !", "CAPTURE LES ZONES"
};

static uint8_t tileMap[MAPW][MAPH];

// ------------------------- Entites ----------------------------------
#define MAX_ENT 14 // place pour les pets invoques (ours, tourelles, stations)
#define MAX_PROJ 64
#define MAX_POP 16
#define MAX_SAFE 4
#define MAX_ZONE 3

struct Ent {
  int brawlerIdx, accIdx, team;
  float x, y, hp, maxHP;
  float ammo, superCharge;
  int gadgetUses;
  bool isHuman, alive;
  int respawnT;
  float regenT;
  float speedBuffT, hyperT, dashT, aimAng, faceAng;
  int dashDx, dashDy;
  int gems, goals, stars, kills;
  bool hasBall;
  int aiThinkT, aiWx, aiWy;
  float aiSkill, stuckT;
  uint8_t aiRole; // 0 = attaquant/buteur, 1 = defenseur, 2 = tueur
  uint8_t aiOnObj; // l'IA est-elle sur son objectif de mode
  int burstLeft, burstTickT; // rafale type Colt : balles qui partent l'une apres l'autre
  float burstAng, burstMul;
  int walkT, lastHitT, attackT;
  bool drawDone;
  bool isPet; int petOwner; uint8_t petKind; float petLife; // creature invoquee (0=ours 1=tourelle 2=station de soin)
  float stealthT, shieldT; // invisibilite (Leon/Sandy) et bouclier (Rosa)
  int upStage; // paliers de Surge
};
static Ent ents[MAX_ENT]; static int entCount = 0;
static int humanIdx = -1;

struct Proj {
  bool active, lob, fromSuper, pierce;
  float x, y, vx, vy, dmg, travelled, range;
  int team, owner, radius, shape, spin;
  uint8_t hitIds[8]; int nHits; // pour les projectiles perforants : touche chaque cible une fois
};
static Proj projs[MAX_PROJ];
static bool gPierceProj = false; // le prochain projectile perce tout (SUP_WAVE)
static float gLobRangeMul = -1.0f; // <0 = portee max ; sinon le lob atterrit sur sa cible
static bool gForceLob = false;     // force le prochain tir en lob (mines de Bo, grenades de Piper)

struct Pop { bool active; float x, y; int life, val; };
static Pop pops[MAX_POP];

struct Gem { bool active; float x, y; };
static Gem gems[10];

struct Safe { float x, y; float hp, maxHP; int team; };
static Safe safes[MAX_SAFE];

struct HotZone { float x, y, prog; };
static HotZone zones[MAX_ZONE];

static float ballX, ballY, ballVX, ballVY;
static int mode, gemCountdown, teamGems[5], teamStars[5];
static int soloShrink, poisonWaveT, matchTimeMs, respawnMs = 3000;
static int shakeT = 0, shakeMag = 0, animT = 0;
static int matchResult = -2;
static int startCountdown = 0; // 3-2-1-GO au debut du match

// ---- Particules (impacts, poussieres, etincelles) ----
#define MAX_PART 40
struct Part { bool active; float x, y, vx, vy; int life; uint8_t kind; };
static Part parts[MAX_PART];
static void spawnParts(float x, float y, int kind, int n) {
  for (int i = 0; i < MAX_PART && n > 0; i++)
    if (!parts[i].active) {
      parts[i].active = true; parts[i].x = x; parts[i].y = y;
      parts[i].vx = (rnd(100) - 50) * 0.6f; parts[i].vy = (rnd(100) - 60) * 0.6f;
      parts[i].life = 300 + rnd(200); parts[i].kind = kind; n--;
    }
}
// ---- Killfeed (dernieres eliminations) ----
#define KILLFEED 3
struct Kill { int alive; int victim, killer; int life; };
static Kill killfeed[KILLFEED];
// (Quest, quests et les compteurs sont declares plus haut, avec saveAcc/loadAcc)
static void addKill(int victim, int killer) {
  if (killer == humanIdx) killCounter++;
  for (int i = KILLFEED - 1; i > 0; i--) killfeed[i] = killfeed[i - 1];
  killfeed[0].alive = 1; killfeed[0].victim = victim; killfeed[0].killer = killer;
  killfeed[0].life = 4000;
}

static void addPop(float x, float y, int val) {
  for (int i = 0; i < MAX_POP; i++)
    if (!pops[i].active) {
      pops[i].active = true; pops[i].x = x; pops[i].y = y;
      pops[i].life = 900; pops[i].val = val;
      return;
    }
}

// ------------------------- Utilitaires monde ------------------------
static bool solidAt(float x, float y) {
  int tx = (int)(x / TILE), ty = (int)(y / TILE);
  if (tx < 0 || ty < 0 || tx >= MAPW || ty >= MAPH) return true;
  return tileMap[tx][ty] == 1;
}
static bool inBush(float x, float y) {
  int tx = (int)(x / TILE), ty = (int)(y / TILE);
  if (tx < 0 || ty < 0 || tx >= MAPW || ty >= MAPH) return false;
  return tileMap[tx][ty] == 2;
}
// destruction de murs par les supers (Shelly, Bull, El Primo, Colt... comme dans BS)
static void breakWalls(float cx, float cy, float r) {
  int t0x = (int)fmaxf(0, (cx - r) / TILE), t1x = (int)fminf(MAPW - 1, (cx + r) / TILE);
  int t0y = (int)fmaxf(0, (cy - r) / TILE), t1y = (int)fminf(MAPH - 1, (cy + r) / TILE);
  for (int tx = t0x; tx <= t1x; tx++)
    for (int ty = t0y; ty <= t1y; ty++) {
      float wx = tx * TILE + TILE / 2.0f, wy = ty * TILE + TILE / 2.0f;
      float dx = wx - cx, dy = wy - cy;
      if (tileMap[tx][ty] == 1 && dx * dx + dy * dy < r * r) {
        tileMap[tx][ty] = 0;
        spawnParts(wx, wy, 1, 6);
      }
    }
}
static void moveEnt(Ent &e, float dx, float dy) {
  if (solidAt(e.x + dx, e.y)) dx = 0;
  if (solidAt(e.x, e.y + dy)) dy = 0;
  e.x += dx; e.y += dy;
  e.x = fmaxf(8, fminf(MAPW * TILE - 8, e.x));
  e.y = fmaxf(8, fminf(MAPH * TILE - 8, e.y));
  if (dx || dy) { e.faceAng = (int)(atan2f(dy, dx) * 57.2957795f) & 255; e.walkT += (int)(fabsf(dx) + fabsf(dy)); }
}

// ------------------------- Generation carte --------------------------
// Rotation des variantes de carte : l'EADK n'expose pas la date, on
// utilise un compteur de matchs (aucun internet, aucune horloge requise).
// Chaque mode possede 3 variantes de disposition, elles tournent a chaque partie.
static int matchCounter = 0;
static int mapDayIndex() {
  return matchCounter % 3;
}
static void genMap() {
  int variant = mapDayIndex();
  uint16_t savedSeed = seed;
  seed = (uint16_t)(12345 + variant * 7919 + mode * 104729); // carte fixe du jour
  for (int x = 0; x < MAPW; x++)
    for (int y = 0; y < MAPH; y++) {
      tileMap[x][y] = 0;
      if (x == 0 || y == 0 || x == MAPW - 1 || y == MAPH - 1) tileMap[x][y] = 1;
    }
  // densite et motif selon la variante du jour
  int density = 95 + variant * 25;        // variante 0 : aeree, 2 : tres bourree
  for (int t = 0; t < density; t++) {
    int x = 1 + rnd(MAPW / 2), y = 1 + rnd(MAPH - 2);
    int v = rnd(10) < (variant == 1 ? 5 : 4) ? 2 : 1;
    if (x < 5 || abs((int)(y - MAPH / 2)) < 2) continue;
    if (variant == 2 && (x % 3 == 0)) continue;   // couloirs alignes
    tileMap[x][y] = v;
    tileMap[MAPW - 1 - x][MAPH - 1 - y] = v;
  }
  // structures symetriques propres a chaque variante
  if (variant >= 1) {
    for (int y = 4; y < MAPH - 4; y++) {          // colonnes laterales
      tileMap[3][y] = 1; tileMap[MAPW - 4][y] = 1;
    }
    for (int i = 0; i < 4; i++) {                 // blocs centraux
      tileMap[MAPW / 2 - 3 + i][3] = 2;
      tileMap[MAPW / 2 - 3 + i][MAPH - 4] = 2;
    }
  }
  if (variant == 2) {                             // buissons protecteurs aux spawn
    for (int i = 0; i < 3; i++) {
      tileMap[5][MAPH / 2 - 3 + i * 3] = 2;
      tileMap[MAPW - 6][MAPH / 2 - 3 + i * 3] = 2;
    }
  }
  if (mode == MODE_BALL || mode == MODE_HEIST || mode == MODE_ZONE)
    for (int x = 1; x < MAPW - 1; x++)
      for (int y = MAPH / 2 - 1; y <= MAPH / 2 + 1; y++) tileMap[x][y] = 0;
  if (mode == MODE_BALL) // bouches de but ouvertes dans le mur de bord : entre les poteaux uniquement
    for (int y = MAPH / 2 - 2; y <= MAPH / 2 + 1; y++) { tileMap[0][y] = 0; tileMap[MAPW - 1][y] = 0; }
  seed = savedSeed; // restaure la graine du match pour l'aléa en jeu
}

// ------------------------- Spawn -------------------------------------
static float aiSkillFromTrophies() {
  float s = totalTrophies() / 1000.0f;
  return fmaxf(0.0f, fminf(1.0f, s));
}

static void addEnt(int bIdx, int accIdx, int team, float x, float y, bool human, float aiSkill) {
  Ent &e = ents[entCount++];
  const BrawlerDef &d = BRAWLERS[bIdx];
  int lvl = 1; bool star = false, gad = false;
  if (accIdx >= 0) {
    lvl = acc.b[accIdx].level; star = acc.b[accIdx].starPower; gad = acc.b[accIdx].gadget;
  } else {
    lvl = 1 + (int)(aiSkill * 9); star = aiSkill > 0.4f; gad = true;
  }
  float mul = lvlMul(lvl);
  if (star && d.star == STAR_HP) mul *= 1.05f;
  e.brawlerIdx = bIdx; e.accIdx = accIdx; e.team = team;
  e.x = x; e.y = y;
  e.maxHP = d.baseHP * mul; e.hp = e.maxHP;
  e.ammo = 3; e.superCharge = 0;
  e.gadgetUses = (accIdx >= 0 && !gad) ? 0 : 2;
  e.isHuman = human; e.alive = true; e.respawnT = 0;
  e.regenT = 0; e.speedBuffT = e.hyperT = e.dashT = 0;
  e.aimAng = e.faceAng = (team == 0) ? 0 : 180;
  e.gems = 0; e.hasBall = false; e.goals = 0; e.stars = 0; e.kills = 0;
  e.aiThinkT = rnd(500); e.aiWx = x; e.aiWy = y;
  e.aiSkill = aiSkill; e.stuckT = 0; e.aiRole = 0; e.aiOnObj = 0;
  e.burstLeft = 0; e.burstTickT = 0; e.burstAng = 0; e.burstMul = 1;
  e.walkT = 0; e.lastHitT = 0; e.attackT = 0; e.drawDone = false;
  e.isPet = false; e.petOwner = -1; e.petKind = 0; e.petLife = 0;
  e.stealthT = 0; e.shieldT = 0; e.upStage = 0;
}

// invoque une creature devant son proprietaire (super de Nita, Jessie, Penny, 8-Bit, Pam)
static void spawnPet(Ent &owner, int kind) {
  if (entCount >= MAX_ENT) return;
  // une seule creature vivante par proprietaire, sauf les rats d'Ash (2 a la fois)
  for (int i = 0; i < entCount; i++)
    if (ents[i].isPet && ents[i].petOwner == (&owner - ents) && ents[i].alive && (kind != 3 || ents[i].petKind != 3)) { ents[i].hp = 0; ents[i].petLife = 0; }
  Ent &e = ents[entCount++];
  e.brawlerIdx = owner.brawlerIdx; e.accIdx = -1; e.team = owner.team;
  e.x = owner.x + cosf(owner.aimAng) * 22; e.y = owner.y + sinf(owner.aimAng) * 22;
  e.maxHP = (kind == 0) ? 4200 : (kind == 1 ? 3200 : (kind == 2 ? 2800 : (kind == 3 ? 2200 : (kind == 4 ? 3000 : 2600))));
  e.hp = e.maxHP; e.ammo = 3; e.superCharge = 0; e.gadgetUses = 0;
  e.isHuman = false; e.alive = true; e.respawnT = 0;
  e.regenT = 0; e.speedBuffT = e.hyperT = e.dashT = 0;
  e.aimAng = e.faceAng = owner.aimAng;
  e.gems = 0; e.hasBall = false; e.goals = 0; e.stars = 0; e.kills = 0;
  e.aiThinkT = 0; e.aiWx = e.x; e.aiWy = e.y;
  e.aiSkill = 0.8f; e.stuckT = 0; e.aiRole = 0; e.aiOnObj = 0;
  e.burstLeft = 0; e.burstTickT = 0; e.burstAng = 0; e.burstMul = 1;
  e.walkT = 0; e.lastHitT = 0; e.attackT = 0; e.drawDone = false;
  e.stealthT = 0; e.shieldT = 0; e.upStage = 0;
  e.isPet = true; e.petOwner = (&owner - ents); e.petKind = (uint8_t)kind;
  e.petLife = (kind == 3) ? 20.0f : ((kind == 4) ? 12.0f : 25.0f);
  spawnParts(e.x, e.y, 1, 6);
}

static void setupMatch(int selMode, int myBrawler) {
  mode = selMode; entCount = 0; humanIdx = -1; matchCounter++;
  for (int i = 0; i < MAX_PROJ; i++) projs[i].active = false;
  for (int i = 0; i < MAX_POP; i++) pops[i].active = false;
  for (int i = 0; i < MAX_PART; i++) parts[i].active = false;
  for (int i = 0; i < KILLFEED; i++) killfeed[i].alive = 0;
  for (int i = 0; i < 10; i++) gems[i].active = false;
  for (int i = 0; i < MAX_SAFE; i++) { safes[i].hp = 0; safes[i].maxHP = 0; }
  for (int i = 0; i < MAX_ZONE; i++) zones[i].prog = 0;
  genMap();
  for (int i = 0; i < 5; i++) { teamGems[i] = 0; teamStars[i] = 0; }
  gemCountdown = -1; matchTimeMs = 0; shakeT = 0; poisonWaveT = 0;
  startCountdown = 3000;
  float skill = aiSkillFromTrophies();

  int pool[55], pn = 0;
  for (int i = 0; i < 55; i++) if (isUnlocked(i)) pool[pn++] = i;
  auto pick = [&]() { return pool[rnd(pn)]; };

  if (mode == MODE_SOLO || mode == MODE_DUO) {
    if (mode == MODE_SOLO) {
      static const float pos[6][2] = {{40,40},{MAPW*TILE-40,40},{40,MAPH*TILE-40},{MAPW*TILE-40,MAPH*TILE-40},{MAPW*TILE/2,30},{MAPW*TILE/2,MAPH*TILE-30}};
      for (int i = 0; i < 6; i++) {
        int bIdx = (i == 0) ? myBrawler : pick();
        addEnt(bIdx, (i == 0) ? myBrawler : -1, i, pos[i][0], pos[i][1], i == 0, skill);
        if (i == 0) humanIdx = 0;
      }
    } else {
      static const float pos[4][2] = {{40,40},{MAPW*TILE-40,40},{40,MAPH*TILE-40},{MAPW*TILE-40,MAPH*TILE-40}};
      addEnt(myBrawler, myBrawler, 0, pos[0][0], pos[0][1] - 25, true, skill); humanIdx = 0;
      addEnt(pick(), -1, 0, pos[0][0], pos[0][1] + 25, false, skill);
      for (int t = 1; t < 4; t++) {
        addEnt(pick(), -1, t, pos[t][0], pos[t][1] - 25, false, skill);
        addEnt(pick(), -1, t, pos[t][0], pos[t][1] + 25, false, skill);
      }
    }
    soloShrink = (MAPW + MAPH) * TILE / 2 + 200;
    poisonWaveT = 60000; // le poison n'arrive qu'apres 1 minute de partie
  } else {
    addEnt(myBrawler, myBrawler, 0, 40, MAPH * TILE / 2, true, skill); humanIdx = 0;
    int a1 = pick(); if (a1 == myBrawler) a1 = pick();
    int a2 = pick(); if (a2 == myBrawler || a2 == a1) a2 = pick();
    addEnt(a1, -1, 0, 40, MAPH * TILE / 2 - 70, false, skill);
    addEnt(a2, -1, 0, 40, MAPH * TILE / 2 + 70, false, skill);
    int e1 = pick(), e2 = pick(), e3 = pick();
    addEnt(e1, -1, 1, MAPW * TILE - 40, MAPH * TILE / 2, false, skill);
    addEnt(e2, -1, 1, MAPW * TILE - 40, MAPH * TILE / 2 - 70, false, skill);
    addEnt(e3, -1, 1, MAPW * TILE - 40, MAPH * TILE / 2 + 70, false, skill);

    if (mode == MODE_GEM)
      for (int i = 0; i < 6; i++) {
        gems[i].active = true;
        gems[i].x = MAPW * TILE / 2 + (rnd(3) - 1) * 30;
        gems[i].y = MAPH * TILE / 2 + (rnd(3) - 1) * 30;
      }
    else if (mode == MODE_BALL) { ballX = MAPW * TILE / 2; ballY = MAPH * TILE / 2; ballVX = ballVY = 0; }
    else if (mode == MODE_HEIST) {
      safes[0].team = 0; safes[0].x = 30; safes[0].y = MAPH * TILE / 2; safes[0].maxHP = 26000; safes[0].hp = safes[0].maxHP;
      safes[1].team = 1; safes[1].x = MAPW * TILE - 30; safes[1].y = MAPH * TILE / 2; safes[1].maxHP = 26000; safes[1].hp = safes[1].maxHP;
    }
    else if (mode == MODE_ZONE) {
      zones[0].x = MAPW * TILE / 2; zones[0].y = MAPH * TILE / 2 - 60;
      zones[1].x = MAPW * TILE / 2 - 70; zones[1].y = MAPH * TILE / 2 + 50;
      zones[2].x = MAPW * TILE / 2 + 70; zones[2].y = MAPH * TILE / 2 + 50;
    }
  }
  // roles IA : dans chaque equipe, le 1er attaque l'objectif, le 2e defend, le 3e chasse
  {
    int memberCount[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < entCount; i++) {
      if (ents[i].isHuman) { ents[i].aiRole = 0; continue; }
      int r = memberCount[ents[i].team]++ % 3;
      ents[i].aiRole = (uint8_t)r;
    }
  }
}

// ------------------------- Projectiles ------------------------------
static void fireProj(Ent &e, float ang, bool isSuper, float dmgMul, float spdMul, float rangeMul) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  for (int i = 0; i < MAX_PROJ; i++) {
    if (projs[i].active) continue;
    Proj &p = projs[i];
    p.active = true; p.lob = (d.atk == ATK_LOB) || gForceLob; p.fromSuper = isSuper;
    p.x = e.x; p.y = e.y;
    float sp = d.projSpeed * spdMul;
    p.vx = cosf(ang) * sp; p.vy = sinf(ang) * sp;
    p.dmg = d.baseDmg * dmgMul;
    p.range = d.range * rangeMul;
    if (p.lob && gLobRangeMul > 0) p.range = p.range * fminf(1.0f, gLobRangeMul); // atterrit sur la cible
    p.travelled = 0; p.team = e.team; p.owner = (&e - ents);
    p.radius = isSuper ? 4 : 2;
    p.pierce = gPierceProj; if (p.pierce) p.radius = 7;
    p.nHits = 0;
    p.shape = d.projShape; p.spin = 0;
    return;
  }
}

static void doAttack(Ent &e, float ang) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  if (e.ammo < 1) return;
  e.aimAng = ang;
  float mul = lvlMul(e.accIdx >= 0 ? acc.b[e.accIdx].level : 1);
  if (e.hyperT > 0) mul *= 1.25f;
  if (e.accIdx >= 0 && acc.b[e.accIdx].starPower && d.star == STAR_DMG) mul *= 1.1f;
  int n = d.projCount + e.upStage, spread = d.spreadDeg; // Surge : +1 balle par palier
  if (n > 1 && d.atk == ATK_SHOT) {
    // tir en rafale (Colt, Rico, Bo...) : les balles partent l'une apres l'autre,
    // pas toutes d'un coup en eventail. La premiere part maintenant.
    fireProj(e, ang, false, mul, 1.0f, 1.0f);
    e.burstLeft = n - 1; e.burstAng = ang; e.burstMul = mul; e.burstTickT = 70;
  } else {
    for (int i = 0; i < n; i++) {
      float a = ang + ((n > 1) ? (spread * 0.01745f * (i - (n - 1) / 2.0f)) : 0);
      fireProj(e, a, false, mul, 1.0f, 1.0f);
    }
  }
  e.ammo -= 1; e.regenT = 0; e.attackT = 200; gLobRangeMul = -1.0f;
}

static void doSuper(Ent &e, float ang) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  if (e.superCharge < 100) return;
  e.superCharge = 0; e.aimAng = ang; e.attackT = 300;
  if (e.isHuman) superCounter++;
  float mul = lvlMul(e.accIdx >= 0 ? acc.b[e.accIdx].level : 1);
  if (e.hyperT > 0) mul *= 1.25f;
  shakeT = 200; shakeMag = 3;
  switch (d.sup) {
    case SUP_BURST: {
      breakWalls(e.x, e.y, 70); // le super dezone : detruit les murs autour (Shelly, Jacky...)
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].team != e.team) {
          float dx = ents[i].x - e.x, dy = ents[i].y - e.y;
          if (dx * dx + dy * dy < 80 * 80) { ents[i].hp -= d.baseDmg * 1.6f * mul; ents[i].regenT = 0; ents[i].lastHitT = 150; }
        }
      for (int a = 0; a < 16; a++) fireProj(e, a * 0.3927f, true, mul * 0.6f, 1.2f, 0.5f);
      break;
    }
    case SUP_DASH: { e.dashT = 0.35f; e.dashDx = (int)(cosf(ang) * 320); e.dashDy = (int)(sinf(ang) * 320); breakWalls(e.x, e.y, 40); break; } // charge qui defonce les murs (Bull, Darryl...)
    case SUP_HEAL: {
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].team == e.team) {
          float before = ents[i].hp;
          ents[i].hp = fminf(ents[i].maxHP, ents[i].hp + ents[i].maxHP * 0.4f);
          if (ents[i].hp > before) addPop(ents[i].x, ents[i].y - 12, (int)(ents[i].hp - before));
        }
      break;
    }
    case SUP_BARRAGE: {
      for (int i = 0; i < 8; i++) fireProj(e, ang + (rnd(30) - 15) * 0.01745f, true, mul, 1.0f, 1.3f);
      break;
    }
    case SUP_MEGASHOT: { // volée perceuse qui defonce les murs (Colt, Brock, Rico...)
      for (int i = 0; i < 5; i++) fireProj(e, ang + (i - 2) * 0.06f, true, mul * 0.75f, 1.3f, 1.6f);
      break;
    }
    case SUP_WAVE: { // onde de choc perforante qui traverse tout (Frank)
      gPierceProj = true;
      fireProj(e, ang, true, mul * 1.5f, 0.9f, 1.4f);
      gPierceProj = false;
      break;
    }
    case SUP_SUMMON: { // creature invoquee (Nita ours, Jessie/8-Bit/Penny tourelle, Pam station, Ash rats, Lola ego, Mr. P pingouins)
      int kind = 1;
      if (e.brawlerIdx == 1) kind = 0; // Nita : ours
      else if (e.brawlerIdx == 21) kind = 2; // Pam : station de soin
      else if (e.brawlerIdx == 32) kind = 3; // Ash : rats
      else if (e.brawlerIdx == 33) kind = 4; // Lola : ego tireur
      else if (e.brawlerIdx == 54) kind = 5; // Mr. P : pingouin
      spawnPet(e, kind);
      if (kind == 3) spawnPet(e, 3); // Ash invoque deux rats
      break;
    }
    case SUP_STEALTH: { // invisibilite d'equipe (Leon, tempete de sable de Sandy)
      for (int i = 0; i < entCount; i++) {
        Ent &a = ents[i];
        if (!a.alive || a.team != e.team || a.isPet) continue;
        float dx = a.x - e.x, dy = a.y - e.y;
        if (dx * dx + dy * dy < 260 * 260) { a.stealthT = 8.0f; spawnParts(a.x, a.y, 1, 5); }
      }
      break;
    }
    case SUP_SHIELD: { e.shieldT = 4.0f; spawnParts(e.x, e.y, 1, 6); break; } // carapace de Rosa : -65% degats
    case SUP_PULL: { // main magique de Gene / trou noir de Tara : attire et blesse
      for (int i = 0; i < entCount; i++) {
        Ent &o = ents[i];
        if (!o.alive || o.team == e.team || o.isPet) continue;
        float dx = e.x - o.x, dy = e.y - o.y, dd = sqrtf(dx * dx + dy * dy);
        if (dd > 20 && dd < 240) {
          float pull = fminf(dd - 40, 130);
          moveEnt(o, dx / dd * pull, dy / dd * pull);
          damageEnt(o, d.baseDmg * mul, e);
        }
      }
      break;
    }
    case SUP_PUSH: { // blizzard de Gale : repousse tout le monde
      for (int i = 0; i < entCount; i++) {
        Ent &o = ents[i];
        if (!o.alive || o.team == e.team || o.isPet) continue;
        float dx = o.x - e.x, dy = o.y - e.y, dd = sqrtf(dx * dx + dy * dy);
        if (dd > 1 && dd < 240) {
          moveEnt(o, dx / dd * 120, dy / dd * 120);
          damageEnt(o, d.baseDmg * 0.6f * mul, e);
        }
      }
      break;
    }
    case SUP_WHIRL: { // Jacky : attire vers elle puis frappe en zone
      for (int i = 0; i < entCount; i++) {
        Ent &o = ents[i];
        if (!o.alive || o.team == e.team || o.isPet) continue;
        float dx = e.x - o.x, dy = e.y - o.y, dd = sqrtf(dx * dx + dy * dy);
        if (dd > 1 && dd < 130) {
          float pull = fmaxf(0, dd - 60);
          moveEnt(o, dx / dd * pull, dy / dd * pull);
          damageEnt(o, d.baseDmg * 1.6f * mul, e);
        }
      }
      shakeT = 250; shakeMag = 4;
      break;
    }
    case SUP_MINES: { // Bo : pose 3 hiboux-mines explosifs autour de sa visee
      gForceLob = true;
      for (int i = 0; i < 3; i++) fireProj(e, ang + (i - 1) * 0.4f, true, mul * 1.2f, 0.7f, 0.5f);
      gForceLob = false;
      break;
    }
    case SUP_UPGRADE: { // Surge : palier suivant (+1 balle par tir, plus rapide)
      if (e.upStage < 2) e.upStage++;
      e.speedBuffT = 4.0f; e.ammo = 3;
      spawnParts(e.x, e.y, 1, 8);
      break;
    }
    case SUP_MECH: { // Meg : transformation en mecha (PV et vitesse augmentes)
      e.maxHP *= 1.35f; e.hp = e.maxHP; e.ammo = 3; e.speedBuffT = 4.0f;
      spawnParts(e.x, e.y, 1, 10);
      break;
    }
    case SUP_LEAP: { // Piper : bond arriere en laissant tomber des grenades
      e.dashT = 0.35f; e.dashDx = (int)(-cosf(ang) * 340); e.dashDy = (int)(-sinf(ang) * 340);
      gForceLob = true;
      for (int i = 0; i < 4; i++) fireProj(e, ang + (rnd(30) - 15) * 0.01745f, true, mul * 0.7f, 0.8f, 0.45f);
      gForceLob = false;
      break;
    }
    case SUP_HASTE: { // Max : coup de vitesse pour toute l'equipe proche
      for (int i = 0; i < entCount; i++) {
        Ent &a = ents[i];
        if (!a.alive || a.team != e.team || a.isPet) continue;
        float dx = a.x - e.x, dy = a.y - e.y;
        if (dx * dx + dy * dy < 280 * 280) { a.speedBuffT = 5.0f; spawnParts(a.x, a.y, 1, 4); }
      }
      break;
    }
    case SUP_WALL: { // Sprout : fait pousser un mur de buissons a la visee
      float wx = e.x + cosf(ang) * fminf(d.range, 200), wy = e.y + sinf(ang) * fminf(d.range, 200);
      int tx = (int)(wx / TILE), ty = (int)(wy / TILE);
      for (int cx2 = tx - 1; cx2 <= tx + 1; cx2++)
        for (int cy2 = ty - 2; cy2 <= ty + 2; cy2++)
          if (cx2 >= 1 && cy2 >= 1 && cx2 < MAPW - 1 && cy2 < MAPH - 1 && tileMap[cx2][cy2] == 0)
            tileMap[cx2][cy2] = 1;
      spawnParts(wx, wy, 2, 10);
      break;
    }
  }
}

static void doGadget(Ent &e) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  if (e.gadgetUses <= 0) return;
  e.gadgetUses--;
  switch (d.gad) {
    case GAD_HEAL: e.hp = fminf(e.maxHP, e.hp + e.maxHP * 0.25f); addPop(e.x, e.y - 12, (int)(e.maxHP * 0.25f)); break;
    case GAD_SPEED: e.speedBuffT = 4.0f; break;
    case GAD_AMMO: e.ammo = 3; break;
    case GAD_BLAST:
      shakeT = 150; shakeMag = 2;
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].team != e.team) {
          float dx = ents[i].x - e.x, dy = ents[i].y - e.y;
          if (dx * dx + dy * dy < 90 * 90) { ents[i].hp -= 900; ents[i].regenT = 0; ents[i].lastHitT = 150; addPop(ents[i].x, ents[i].y - 12, 900); }
        }
      break;
  }
}

static void doHypercharge(Ent &e) {
  if (e.accIdx < 0 || !acc.b[e.accIdx].hyper) return;
  if (e.superCharge < 100) return;
  e.superCharge = 0;
  e.hyperT = 8.0f; e.speedBuffT = 8.0f;
  shakeT = 300; shakeMag = 4;
}

// ------------------------- Clavier ------------------------------------
// Commandes (V5.3, nouveau schema) :
//   Fleches : bouger | OK ou var : attaque vers la direction du perso
//   RETOUR (back) : ulti (super) | SHIFT : gadget | EFFACER : hypercharge
//   HOME : quitter le match. Le maintien de OK/var tire en continu.
static bool keyOK, keyBack, prevOK, prevShift, prevMinus, prevBack;
static bool kUp, kDown, kLeft, kRight, kShift, kBacksp, kHome, kMinus, kVar, kExp;
static bool kPlus, kMul, kDiv, kPow;
static bool prevVar, prevExp, prevBacksp;

static void pollKeys() {
  Ion::Keyboard::State st = Ion::Keyboard::getState();
  using K = Ion::Keyboard::Key;
  kUp = st.keyDown(K::Up); kDown = st.keyDown(K::Down);
  kLeft = st.keyDown(K::Left); kRight = st.keyDown(K::Right);
  keyOK = st.keyDown(K::OK);
  kShift = st.keyDown(K::Shift); // gadget
  kBacksp = st.keyDown(K::Backspace) || st.keyDown(K::X); // hypercharge (effacer)
  kHome = st.keyDown(K::Home);
  kVar = st.keyDown(K::Var);
  kExp = st.keyDown(K::Toolbox) || st.keyDown(K::EXE); // alias d'attaque
  kMinus = st.keyDown(K::Minus); keyBack = st.keyDown(K::Back); // back = ulti en match
  kPlus = st.keyDown(K::Plus); kMul = st.keyDown(K::Multiply);
  kDiv = st.keyDown(K::Division); kPow = st.keyDown(K::Power);
}
// anti double-appui : attend que les touches d'action soient relachees
static void waitRelease() {
  while (true) {
    pollKeys();
    if (!keyOK && !keyBack && !kShift && !kBacksp && !kVar && !kExp && !kMinus) return;
    Ion::Timing::msleep(30);
  }
}

// ------------------------- IA ---------------------------------------
static float nearestEnemyDist(Ent &e, Ent **out) {
  float best = 1e9; *out = nullptr;
  for (int i = 0; i < entCount; i++) {
    Ent &o = ents[i];
    if (!o.alive || o.team == e.team) continue;
    if (inBush(o.x, o.y) && !e.isHuman) continue;
    if (o.stealthT > 0) continue; // invisible : impossible a cibler
    float dx = o.x - e.x, dy = o.y - e.y, d2 = dx * dx + dy * dy;
    if (d2 < best) { best = d2; *out = &o; }
  }
  return sqrtf(best);
}

static void updateAI(Ent &e, float dt) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  float sk = e.aiSkill;
  bool melee = d.range < 130;
  Ent *tgt = nullptr;
  float ed = nearestEnemyDist(e, &tgt);

  // ---------- 1) OBJECTIF : on avance TOUJOURS, pas besoin d'ennemi a portee ----------
  e.aiThinkT -= (int)(dt * 1000);
  if (e.aiThinkT <= 0) {
    e.aiThinkT = (int)(450 - 250 * sk) + rnd(200);
    float tx = e.x, ty = e.y;
    bool onObj = false;
    if (mode == MODE_GEM) {
      // la gemme la plus PRECHE, pas la premiere de la liste
      float bestD = 1e9;
      for (int i = 0; i < 10; i++)
        if (gems[i].active) {
          float ddx = gems[i].x - e.x, ddy = gems[i].y - e.y, dd = ddx * ddx + ddy * ddy;
          if (dd < bestD) { bestD = dd; tx = gems[i].x; ty = gems[i].y; }
        }
      // compte a rebours : on protege son butin en reculant dans son camp
      if (gemCountdown > 0 && bestD > 100 * 100) { tx = (e.team == 0) ? 60 : MAPW * TILE - 60; ty = MAPH * TILE / 2; }
      else if (bestD == 1e9) { tx = MAPW * TILE / 2; ty = MAPH * TILE / 2; }
      onObj = (bestD != 1e9);
    }
    else if (mode == MODE_BALL) {
      // roles Brawl Ball : 0 = buteur, 1 = defenseur, 2 = tueur
      float goalX = (e.team == 0) ? MAPW * TILE - 12 : 12;
      float ownX  = (e.team == 0) ? 20 : MAPW * TILE - 20;
      if (e.hasBall) { tx = goalX; ty = MAPH * TILE / 2; } // porteur : fonce marquer, meuble rien
      else if (e.aiRole == 1) { // defenseur : veille devant son but
        tx = ownX + ((e.team == 0) ? 45 : -45);
        ty = MAPH * TILE / 2 + ((e.walkT / 600) % 2 ? 50 : -50);
      }
      else if (e.aiRole == 2) { /* tueur : cible l'ennemi via tgt plus bas */ }
      else if (ballVX == 0 && ballVY == 0) { tx = ballX; ty = ballY; } // ballon libre : on va le prendre
      else { tx = (goalX + ballX) / 2; ty = ballY; } // soutien : accompagne le porteur
      onObj = true;
    }
    else if (mode == MODE_HEIST) {
      // on pique le coffre adverse, en s'arretant a portee de tir (on n'embrasse pas le coffre)
      for (int s = 0; s < MAX_SAFE; s++)
        if (safes[s].maxHP > 0 && safes[s].hp > 0 && safes[s].team != e.team) {
          float ddx = safes[s].x - e.x, ddy = safes[s].y - e.y, dd = sqrtf(ddx * ddx + ddy * ddy);
          float back = fmaxf(0, dd - d.range * 0.8f);
          tx = safes[s].x - (dd > 1 ? ddx / dd * back : 0);
          ty = safes[s].y - (dd > 1 ? ddy / dd * back : 0);
        }
      onObj = true;
    }
    else if (mode == MODE_ZONE) {
      float bestD = 1e9; bool found = false;
      for (int z = 0; z < MAX_ZONE; z++) {
        bool mine = (e.team == 0) ? zones[z].prog >= 100 : zones[z].prog <= -100;
        if (mine) continue;
        float ddx = zones[z].x - e.x, ddy = zones[z].y - e.y, dd = ddx * ddx + ddy * ddy;
        if (dd < bestD) { bestD = dd; found = true; tx = zones[z].x; ty = zones[z].y; }
      }
      if (!found) {
        float bestP = 1e9;
        for (int z = 0; z < MAX_ZONE; z++)
          if (zones[z].prog < bestP) { bestP = zones[z].prog; tx = zones[z].x; ty = zones[z].y; }
      }
      onObj = true;
    }
    else { // RECOMPENSE, KO, SURVIVANT : chasse active vers le centre ou l'ennemi connu
      if (tgt) { tx = tgt->x; ty = tgt->y; }
      else { tx = MAPW * TILE / 2; ty = MAPH * TILE / 2; }
      onObj = (tgt != nullptr);
    }
    // anti-embouteillage : si un coequipier vise deja ce point, on se decale
    for (int i = 0; i < entCount; i++) {
      Ent &a = ents[i];
      if (&a == &e || !a.alive || a.isPet || a.team != e.team) continue;
      float ddx = a.x - tx, ddy = a.y - ty;
      if (ddx * ddx + ddy * ddy < 60 * 60) { tx += (rnd(80) - 40); ty += (rnd(80) - 40); break; }
    }
    e.aiWx = tx; e.aiWy = ty;
    e.aiOnObj = onObj;
  }

  // ---------- 2) SURVIE : on decroche quand on est trop amoché ----------
  bool low = e.hp < e.maxHP * (0.38f - 0.15f * sk);
  float wx = e.aiWx, wy = e.aiWy;
  if (low && tgt && ed < 240 && e.gadgetUses <= 0) {
    // repli vers son coin de depart en tirant en reculant si possible
    wx = (e.team == 0) ? 30 : MAPW * TILE - 30;
    wy = MAPH * TILE / 2 + (rnd(120) - 60);
  }

  // fuir la zone de poison : retour vers le centre
  if (mode == MODE_SOLO || mode == MODE_DUO || mode == MODE_KO) {
    int cx = MAPW * TILE / 2, cy = MAPH * TILE / 2;
    int ddx = e.x - cx, ddy = e.y - cy;
    if (ddx * ddx + ddy * ddy > (soloShrink - 40) * (soloShrink - 40)) { wx = cx; wy = cy; }
  }

  // ---------- 3) COMBAT : on tire a chaque occasion, en bougeant ----------
  float angT = e.aimAng;
  if (tgt) {
    angT = atan2f(tgt->y - e.y, tgt->x - e.x);
    float err = (1.0f - sk) * 12.0f;
    if (ed < d.range * 0.95f && e.ammo >= 1 && e.attackT <= 0 && !(mode == MODE_BALL && e.hasBall)) {
      if (d.atk == ATK_LOB) gLobRangeMul = fminf(1.0f, ed / d.range);
      doAttack(e, angT + (rnd(100) / 100.0f - 0.5f) * err * 0.01745f);
      gLobRangeMul = -1.0f;
    }
    // kite : les tireurs gardent leurs distances, les melees collent la cible
    if (!melee && !low && ed < d.range * 0.5f) {
      float l = fmaxf(ed, 1);
      wx = e.x - (tgt->x - e.x) / l * 90; wy = e.y - (tgt->y - e.y) / l * 90;
    } else if (tgt && e.aiOnObj && ed < d.range * 0.85f) {
      wx = e.aiWx; wy = e.aiWy; // on reste sur l'objectif en canardant
    }
  }
  // IA porteuse du ballon : fonce au but ET shoote sur les defenseurs proches
  if (mode == MODE_BALL && e.hasBall) {
    wx = (e.team == 0) ? MAPW * TILE - 20 : 20; wy = MAPH * TILE / 2;
    if (tgt && ed < d.range * 0.8f && e.ammo >= 1 && e.attackT <= 0)
      doAttack(e, atan2f(tgt->y - e.y, tgt->x - e.x));
  }

  // ---------- 4) SUPER : au bon moment et au bon endroit ----------
  if (e.superCharge >= 100) {
    bool done = false;
    switch (d.sup) {
      // supers de zone : on les pose sur l'objectif, ennemi ou pas
      case SUP_SUMMON: case SUP_WALL: case SUP_MINES:
        if (e.aiOnObj && (rnd(100) < 40 * sk)) { doSuper(e, angT); done = true; }
        break;
      // supers de fuite/survie : quand ca chauffe
      case SUP_STEALTH: case SUP_MECH:
        if (low) { doSuper(e, angT); done = true; } break;
      case SUP_SHIELD:
        if (tgt && ed < d.range * 1.2f) { doSuper(e, angT); done = true; } break;
      case SUP_HASTE: case SUP_UPGRADE: case SUP_HEAL:
        if (rnd(100) < 25) { doSuper(e, angT); done = true; } break;
      default: // supers d'attaque : seulement a portee d'une cible
        if (tgt && ed < d.range) {
          if (d.atk == ATK_LOB) gLobRangeMul = fminf(1.0f, ed / d.range);
          doSuper(e, angT); gLobRangeMul = -1.0f; done = true;
        }
        break;
    }
    (void)done;
  }

  // ---------- 5) GADGET : defensif si blesse, offensif si l'ennemi colle ----------
  if (e.gadgetUses > 0 && e.attackT <= 0) {
    if (e.hp < e.maxHP * 0.35f) doGadget(e);
    else if (d.gad == GAD_BLAST && tgt && ed < 70 && sk > 0.3f) doGadget(e);
    else if (d.gad == GAD_SPEED && low && tgt && ed < 200) doGadget(e);
  }

  // ---------- 6) DEPLACEMENT ----------
  float dx = wx - e.x, dy = wy - e.y;
  float len = sqrtf(dx * dx + dy * dy);
  if (len > 10) {
    float sp = 90 + 30 * sk;
    float ox = e.x, oy = e.y;
    moveEnt(e, dx / len * sp * dt, dy / len * sp * dt);
    e.walkT += (int)(dt * 1000);
    // anti-blocage : si on n'a presque pas bouge, on contourne par un cote
    float moved = fabsf(e.x - ox) + fabsf(e.y - oy);
    if (moved < sp * dt * 0.3f) {
      e.stuckT += dt;
      if (e.stuckT > 0.35f) {
        e.stuckT = 0;
        float nx = -dy / len, ny = dx / len;
        int side = rnd(2) ? 1 : -1;
        e.aiWx = (int)(e.x + nx * side * 80);
        e.aiWy = (int)(e.y + ny * side * 80);
        e.aiThinkT = 600;
      }
    } else e.stuckT = 0;
  } else e.stuckT = 0;
}

// ------------------------- Match --------------------------------------
// Tir du ballon porte : le "tir" du porteur envoie le ballon (Brawl Ball)
static void shootBall(Ent &e, float ang) {
  e.hasBall = false;
  ballX = e.x + cosf(ang) * 12; ballY = e.y + sinf(ang) * 12;
  ballVX = cosf(ang) * 300; ballVY = sinf(ang) * 300;
  e.attackT = 200; shakeT = 120; shakeMag = 2;
}
// But marque par le ballon libre : credite l'equipe attaquante et recentre
static void teamGoal(int team) {
  int rec = -1;
  for (int i = 0; i < entCount; i++)
    if (ents[i].team == team && (rec < 0 || i == humanIdx)) rec = i;
  if (rec >= 0) ents[rec].goals++;
  ballX = MAPW * TILE / 2; ballY = MAPH * TILE / 2; ballVX = ballVY = 0;
  shakeT = 400; shakeMag = 5;
}
static void damageEnt(Ent &e, float dmg, Ent &src) {
  if (e.shieldT > 0) dmg *= 0.35f; // carapace de Rosa active
  e.hp -= dmg; e.regenT = 0; e.lastHitT = 150;
  src.superCharge = fminf(100, src.superCharge + (src.hyperT > 0 ? 3.5f : 2.0f));
  addPop(e.x + rnd(8) - 4, e.y - 12, (int)dmg);
  spawnParts(e.x, e.y, 0, 3);
}

// IA des creatures invoquees : ours/rats/pingouins melee, tourelle/ego tireurs, station de soin
static void updatePet(Ent &e, float dt) {
  e.petLife -= dt;
  if (e.petLife <= 0) { e.hp = 0; return; } // expiration : part en particules via le bloc de mort
  if (e.petKind == 0 || e.petKind == 3 || e.petKind == 5) { // chasseurs au corps a corps (ours, rats, pingouin)
    float spd = (e.petKind == 3) ? 200.0f : ((e.petKind == 5) ? 170.0f : 150.0f);
    float dmg = (e.petKind == 3) ? 550.0f : ((e.petKind == 5) ? 700.0f : 950.0f);
    Ent *t = nullptr; nearestEnemyDist(e, &t);
    if (t) {
      float dx = t->x - e.x, dy = t->y - e.y, l = sqrtf(dx * dx + dy * dy);
      if (l > 26) { moveEnt(e, dx / l * spd * dt, dy / l * spd * dt); e.walkT += (int)(dt * 1000); }
      else if (e.attackT <= 0) {
        e.aimAng = e.faceAng = atan2f(dy, dx);
        damageEnt(*t, dmg, ents[e.petOwner]);
        e.attackT = 500;
      }
    }
  } else if (e.petKind == 1 || e.petKind == 4) { // tireurs : tourelle immobile, ego de Lola mobile
    Ent *t = nullptr; float ed = nearestEnemyDist(e, &t);
    if (e.petKind == 4 && t && ed > 220) { // l'ego avance pour rester a portee
      float dx = t->x - e.x, dy = t->y - e.y;
      moveEnt(e, dx / ed * 120 * dt, dy / ed * 120 * dt); e.walkT += (int)(dt * 1000);
    }
    if (t && ed < 260 && e.attackT <= 0 && e.ammo >= 1) {
      float ang = atan2f(t->y - e.y, t->x - e.x);
      e.aimAng = e.faceAng = ang;
      fireProj(e, ang, false, 1.0f, 1.0f, 0.7f);
      e.attackT = 600;
    }
  } else { // station de soin de Pam : regenere les allies proches
    for (int i = 0; i < entCount; i++) {
      Ent &a = ents[i];
      if (!a.alive || a.team != e.team || a.isPet) continue;
      float dx = a.x - e.x, dy = a.y - e.y;
      if (dx * dx + dy * dy < 90 * 90) {
        float before = a.hp;
        a.hp = fminf(a.maxHP, a.hp + a.maxHP * 0.10f * dt);
        if ((int)(a.hp / 200) != (int)(before / 200) && a.hp > before) addPop(a.x, a.y - 12, (int)(a.hp - before));
      }
    }
  }
}

static void updateMatch(float dt) {
  if (startCountdown > -800) {
    // phase 3-2-1-GO : le temps tourne uniquement pour l'anim
    startCountdown -= (int)(dt * 1000);
    animT += (int)(dt * 1000);
    return;
  }
  matchTimeMs += (int)(dt * 1000);
  animT += (int)(dt * 1000);
  if (shakeT > 0) shakeT -= (int)(dt * 1000);
  bool ffa = (mode == MODE_SOLO || mode == MODE_DUO);
  bool noRespawn = ffa || mode == MODE_KO;

  for (int i = 0; i < MAX_POP; i++)
    if (pops[i].active) { pops[i].life -= (int)(dt * 1000); pops[i].y -= dt * 22; if (pops[i].life <= 0) pops[i].active = false; }
  for (int i = 0; i < MAX_PART; i++)
    if (parts[i].active) {
      parts[i].life -= (int)(dt * 1000);
      parts[i].x += parts[i].vx * dt; parts[i].y += parts[i].vy * dt;
      parts[i].vy += 60 * dt;
      if (parts[i].life <= 0) parts[i].active = false;
    }
  for (int i = 0; i < KILLFEED; i++)
    if (killfeed[i].alive) { killfeed[i].life -= (int)(dt * 1000); if (killfeed[i].life <= 0) killfeed[i].alive = 0; }

  Ent &me = ents[humanIdx];
  pollKeys();
  if (kHome) { matchResult = 0; return; } // HOME quitte le match (RETOUR sert d'ulti)
  if (me.alive) {
    float spd = 110.0f;
    if (me.speedBuffT > 0) spd *= 1.3f;
    if (me.hyperT > 0) spd *= 1.15f;
    const BrawlerDef &d = BRAWLERS[me.brawlerIdx];
    if (me.accIdx >= 0 && acc.b[me.accIdx].starPower && d.star == STAR_SPEED) spd *= 1.1f;
    float dx = (kRight ? 1 : 0) - (kLeft ? 1 : 0);
    float dy = (kDown ? 1 : 0) - (kUp ? 1 : 0);
    if (dx || dy) { float l = sqrtf(dx * dx + dy * dy); moveEnt(me, dx / l * spd * dt, dy / l * spd * dt); }
    // --- Commandes V5.3 : OK/var = attaque vers la direction du perso,
    //     RETOUR = ulti, SHIFT = gadget, EFFACER = hypercharge, HOME = quitter ---
    if (kBacksp && !prevBacksp) doHypercharge(me);
    bool shotGad = kShift && !prevShift;
    bool shotUlti = keyBack && !prevBack;
    bool shotAtk = (keyOK && !prevOK) || (kVar && !prevVar) || (kExp && !prevExp);
    // maintien de OK/var/menu = tir continu (respecte le cooldown d'attaque)
    if (me.attackT <= 0 && me.ammo >= 1 && (keyOK || kVar || kExp)) shotAtk = true;
    // le tir va vers la direction du perso ; a l'arret, vers l'ennemi le plus proche
    Ent *tgt = nullptr; nearestEnemyDist(me, &tgt);
    float ang = (dx || dy) ? atan2f(dy, dx)
              : (tgt ? atan2f(tgt->y - me.y, tgt->x - me.x) : me.aimAng);
    if (shotUlti) { if (me.superCharge >= 100) doSuper(me, ang); }
    if (shotGad) doGadget(me);
    if (shotAtk) {
      if (mode == MODE_BALL && me.hasBall) {
        shootBall(me, ang); // on tire le ballon au lieu d'attaquer
      } else {
        // lanceur : le tir atterrit sur l'ennemi le plus proche si on est a l'arret
        if (d.atk == ATK_LOB && !(dx || dy) && tgt) gLobRangeMul = fminf(1.0f, sqrtf((tgt->x-me.x)*(tgt->x-me.x)+(tgt->y-me.y)*(tgt->y-me.y)) / d.range);
        doAttack(me, ang);
        gLobRangeMul = -1.0f;
      }
    }
  }
  prevOK = keyOK; prevShift = kShift; prevMinus = kMinus;
  prevVar = kVar; prevExp = kExp; prevBacksp = kBacksp;

  for (int i = 0; i < entCount; i++) {
    Ent &e = ents[i];
    if (!e.alive) {
      e.respawnT -= (int)(dt * 1000);
      if (e.respawnT <= 0 && !noRespawn) {
        e.alive = true; e.hp = e.maxHP; e.ammo = 3;
        e.x = (e.team == 0) ? 40 : MAPW * TILE - 40;
        e.y = MAPH * TILE / 2 + (rnd(3) - 1) * 60;
      }
      continue;
    }
    if (!e.isHuman && !e.isPet) updateAI(e, dt);
    else if (e.isPet) updatePet(e, dt);
    if (e.speedBuffT > 0) e.speedBuffT -= dt;
    if (e.stealthT > 0) e.stealthT -= dt;
    if (e.shieldT > 0) e.shieldT -= dt;
    if (e.hyperT > 0) e.hyperT -= dt;
    if (e.lastHitT > 0) e.lastHitT -= (int)(dt * 1000);
    if (e.attackT > 0) e.attackT -= (int)(dt * 1000);
    // rafale en cours (Colt, Rico...) : la balle suivante part apres un court delai
    if (e.burstLeft > 0) {
      e.burstTickT -= (int)(dt * 1000);
      if (e.burstTickT <= 0) {
        fireProj(e, e.burstAng, false, e.burstMul, 1.0f, 1.0f);
        e.burstLeft--; e.burstTickT = 70;
      }
    }
    if (e.dashT > 0) {
      e.dashT -= dt;
      breakWalls(e.x, e.y, 14); // la charge defonce les murs sur son passage
      moveEnt(e, e.dashDx * dt, e.dashDy * dt);
      for (int j = 0; j < entCount; j++)
        if (ents[j].alive && ents[j].team != e.team) {
          float ddx = ents[j].x - e.x, ddy = ents[j].y - e.y;
          if (ddx * ddx + ddy * ddy < 20 * 20) damageEnt(ents[j], 2200, e);
        }
    }
    const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
    e.ammo = fminf(3, e.ammo + dt * 1000.0f / d.reloadMs);
    e.regenT += dt;
    float regenDelay = 4.0f;
    if (e.accIdx >= 0 && acc.b[e.accIdx].starPower && d.star == STAR_REGEN) regenDelay = 2.0f;
    if (e.regenT > regenDelay) e.hp = fminf(e.maxHP, e.hp + e.maxHP * 0.12f * dt);
    if (e.hyperT > 0) e.superCharge = fminf(100, e.superCharge + 6 * dt);
    if (e.hp <= 0) {
      e.alive = false;
      if (e.isPet) { // une creature morte ne respawn pas et ne rapporte rien
        e.respawnT = 99999999;
        spawnParts(e.x, e.y, 1, 8);
        continue;
      }
      e.respawnT = respawnMs;
      if (e.gems > 0)
        for (int g = 0; g < 10 && e.gems > 0; g++)
          if (!gems[g].active) { gems[g].active = true; gems[g].x = e.x + rnd(20) - 10; gems[g].y = e.y + rnd(20) - 10; e.gems--; }
      if (e.hasBall) { e.hasBall = false; ballX = e.x; ballY = e.y; }
      shakeT = 200; shakeMag = 3;
      spawnParts(e.x, e.y, 1, 8);
      for (int j = 0; j < entCount; j++)
        if (ents[j].alive && ents[j].team != e.team && fabsf(ents[j].x - e.x) < 140 && fabsf(ents[j].y - e.y) < 140) { addKill((&e - ents), j); break; }
      if (ffa && e.isHuman) matchResult = -1;
      if (mode == MODE_BOUNTY) {
        int killer = 1;
        for (int j = 0; j < entCount; j++)
          if (ents[j].alive && ents[j].team != e.team && fabsf(ents[j].x - e.x) < 140 && fabsf(ents[j].y - e.y) < 140) { killer = ents[j].team; break; }
        int gained = 1 + (e.stars > 2 ? 2 : e.stars);
        teamStars[killer] += gained;
      }
    }
  }

  for (int i = 0; i < MAX_PROJ; i++) {
    Proj &p = projs[i];
    if (!p.active) continue;
    p.spin += 30;
    float nx = p.x + p.vx * dt, ny = p.y + p.vy * dt;
    p.travelled += sqrtf((nx - p.x) * (nx - p.x) + (ny - p.y) * (ny - p.y));
    p.x = nx; p.y = ny;
    if (!p.lob && p.travelled > p.range) { p.active = false; continue; } // le lob, lui, explose (voir plus bas)
    // anti-tunneling : milieu du pas teste aussi. Les supers perceurs cassent
    // les murs a l'impact (Colt, Brock, Rico...), les tirs normaux s'y arretent.
    if (!p.lob) {
      bool hitWall = solidAt(p.x, p.y) || solidAt(p.x - p.vx * dt * 0.5f, p.y - p.vy * dt * 0.5f);
      if (hitWall) {
        if (p.fromSuper) breakWalls(p.x, p.y, 16);
        else { p.active = false; continue; }
      }
    }
    if (mode == MODE_HEIST)
      for (int s = 0; s < MAX_SAFE; s++) {
        if (safes[s].hp <= 0 || safes[s].team == p.team) continue;
        float ddx = safes[s].x - p.x, ddy = safes[s].y - p.y;
        if (ddx * ddx + ddy * ddy < (p.radius + 12) * (p.radius + 12)) {
          safes[s].hp -= p.dmg;
          addPop(safes[s].x, safes[s].y - 14, (int)p.dmg);
          shakeT = 100; shakeMag = 2;
          p.active = false; break;
        }
      }
    if (!p.active) continue;
    // les lanceurs (ATK_LOB) : le projectile passe au-dessus des murs et des
    // ennemis, puis explose en ZONE a l'atterrissage (comme dans Brawl Stars)
    if (p.lob) {
      if (p.travelled >= p.range) {
        int R = p.fromSuper ? 40 : 30;
        spawnParts(p.x, p.y, 0, 10);
        shakeT = 100; shakeMag = 2;
        for (int j = 0; j < entCount; j++) {
          Ent &t = ents[j];
          if (!t.alive || t.team == p.team) continue;
          float ddx = t.x - p.x, ddy = t.y - p.y;
          if (ddx * ddx + ddy * ddy < R * R) damageEnt(t, p.dmg, ents[p.owner]);
        }
        if (mode == MODE_HEIST)
          for (int s = 0; s < MAX_SAFE; s++) {
            if (safes[s].hp <= 0 || safes[s].team == p.team) continue;
            float ddx = safes[s].x - p.x, ddy = safes[s].y - p.y;
            if (ddx * ddx + ddy * ddy < (R + 12) * (R + 12)) {
              safes[s].hp -= p.dmg;
              addPop(safes[s].x, safes[s].y - 14, (int)p.dmg);
            }
          }
        p.active = false;
      }
      continue;
    }
    for (int j = 0; j < entCount; j++) {
      Ent &t = ents[j];
      if (!t.alive || t.team == p.team) continue;
      float ddx = t.x - p.x, ddy = t.y - p.y;
      if (ddx * ddx + ddy * ddy < (p.radius + 7) * (p.radius + 7)) {
        if (p.pierce) { // onde perforante : chaque cible touchee une seule fois
          bool already = false;
          for (int k = 0; k < p.nHits; k++) if (p.hitIds[k] == j) { already = true; break; }
          if (already) continue;
          if (p.nHits < 8) p.hitIds[p.nHits++] = j;
          damageEnt(t, p.dmg, ents[p.owner]);
        } else {
          damageEnt(t, p.dmg, ents[p.owner]);
          p.active = false; break;
        }
      }
    }
  }

  if (mode == MODE_GEM) {
    for (int t = 0; t < 5; t++) teamGems[t] = 0;
    for (int i = 0; i < entCount; i++) if (ents[i].alive) teamGems[ents[i].team] += ents[i].gems;
    for (int g = 0; g < 10; g++) {
      if (!gems[g].active) continue;
      for (int i = 0; i < entCount; i++)
        if (!ents[i].isPet && ents[i].alive && fabsf(ents[i].x - gems[g].x) < 12 && fabsf(ents[i].y - gems[g].y) < 12) {
          gems[g].active = false; ents[i].gems++;
          if (i == humanIdx) gemCounter++;
          break;
        }
      int actives = 0;
      for (int q = 0; q < 10; q++) actives += gems[q].active;
      if (actives == 0 && rnd(300) < 6) {
        gems[g].active = true;
        gems[g].x = MAPW * TILE / 2 + (rnd(3) - 1) * 40;
        gems[g].y = MAPH * TILE / 2 + (rnd(3) - 1) * 40;
      }
    }
    if (teamGems[0] >= 10 || teamGems[1] >= 10) {
      if (gemCountdown < 0) gemCountdown = 15000;
      gemCountdown -= (int)(dt * 1000);
      if (gemCountdown <= 0) {
        // fin du compte a rebours : l'equipe qui a 10+ gemmes gagne, sinon egalite
        if (teamGems[0] >= 10 && teamGems[1] >= 10) matchResult = (teamGems[0] >= teamGems[1]) ? 1 : -1;
        else if (teamGems[0] >= 10) matchResult = 1;
        else if (teamGems[1] >= 10) matchResult = -1;
        else matchResult = 0;
      }
    } else gemCountdown = -1;
  }

  if (mode == MODE_BALL) {
    bool carried = false;
    for (int i = 0; i < entCount; i++) if (ents[i].alive && ents[i].hasBall) carried = true;
    if (!carried) {
      for (int i = 0; i < entCount; i++) {
        Ent &e = ents[i];
        if (e.isPet || !e.alive || e.hasBall) continue;
        if (fabsf(e.x - ballX) < 12 && fabsf(e.y - ballY) < 12) { e.hasBall = true; break; }
      }
      if (!carried) {
        ballX += ballVX * dt; ballY += ballVY * dt;
        ballVX *= 0.95f; ballVY *= 0.95f;
        if (solidAt(ballX, ballY)) { ballX -= ballVX * dt * 2; ballY -= ballVY * dt * 2; ballVX = ballVY = 0; }
        // ballon libre qui entre dans un but (entre les poteaux) : but pour l'equipe qui attaque ce cote
        int goalCY = MAPH * TILE / 2;
        if (fabsf(ballY - goalCY) < 2.2f * TILE) {
          if (ballX > MAPW * TILE - 14) { teamGoal(0); }
          else if (ballX < 14) { teamGoal(1); }
        } else {
          if (ballX > MAPW * TILE - 14) { ballX = MAPW * TILE - 14; ballVX = 0; }
          if (ballX < 14) { ballX = 14; ballVX = 0; }
        }
      }
    } else {
      int goalCY = MAPH * TILE / 2;
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].hasBall) {
          ballX = ents[i].x; ballY = ents[i].y;
          bool inGoalMouth = fabsf(ballY - goalCY) < 2.2f * TILE; // dans les poteaux, pas au-dessus/dessous
          if ((ents[i].team == 0 && ballX > MAPW * TILE - 26) ||
              (ents[i].team == 1 && ballX < 26)) {
            if (inGoalMouth) {
              ents[i].goals++; ents[i].hasBall = false;
              ballX = MAPW * TILE / 2; ballY = MAPH * TILE / 2; ballVX = ballVY = 0;
              shakeT = 400; shakeMag = 5;
            } else { // hors des poteaux : on reste dans le terrain
              ballX = ents[i].x = (ents[i].team == 0) ? MAPW * TILE - 26 : 26;
            }
          }
        }
    }
    int g0 = 0, g1 = 0;
    for (int i = 0; i < entCount; i++) {
      if (ents[i].team == 0) g0 = fmaxf(g0, ents[i].goals); else g1 = fmaxf(g1, ents[i].goals);
    }
    if (g0 >= 2) matchResult = 1;
    if (g1 >= 2) matchResult = -1;
  }

  if (mode == MODE_HEIST) {
    for (int s = 0; s < MAX_SAFE; s++) {
      if (safes[s].maxHP > 0 && safes[s].hp <= 0)
        matchResult = (s == 1) ? 1 : -1;
    }
  }

  if (mode == MODE_BOUNTY) {
    if (teamStars[0] >= 20) matchResult = 1;
    if (teamStars[1] >= 20) matchResult = -1;
  }

  if (mode == MODE_ZONE) {
    for (int z = 0; z < MAX_ZONE; z++) {
      int myTeamHere = 0, enTeamHere = 0;
      for (int i = 0; i < entCount; i++) {
        if (!ents[i].alive || ents[i].isPet) continue; // les tourelles ne capturent pas
        if (fabsf(ents[i].x - zones[z].x) < 24 && fabsf(ents[i].y - zones[z].y) < 24) {
          if (ents[i].team == 0) myTeamHere++; else enTeamHere++;
        }
      }
      if (myTeamHere > 0 && enTeamHere == 0) zones[z].prog = fminf(100, zones[z].prog + 9 * dt);
      if (enTeamHere > 0 && myTeamHere == 0) zones[z].prog = fmaxf(-100, zones[z].prog - 9 * dt);
      if (zones[z].prog >= 100) matchResult = 1;
      if (zones[z].prog <= -100) matchResult = -1;
    }
  }

  if (mode == MODE_KO || ffa) {
    poisonWaveT -= (int)(dt * 1000);
    if (poisonWaveT <= 0) {
      // vague lente : retrait progressif, bloque au minimum (= poison sur toute la map)
      poisonWaveT = 7000;
      soloShrink -= (mode == MODE_KO) ? 50 : 35;
      if (soloShrink < 80) soloShrink = 80; // toute la map empoisonnee : ca reste jusqu'au gagnant
    }
    int cx = MAPW * TILE / 2, cy = MAPH * TILE / 2;
    for (int i = 0; i < entCount; i++) {
      Ent &e = ents[i];
      if (!e.alive) continue;
      int ddx = e.x - cx, ddy = e.y - cy;
      if (ddx * ddx + ddy * ddy > soloShrink * soloShrink) { e.hp -= 700 * dt; e.regenT = 0; }
    }
    if (mode == MODE_KO) {
      int alive0 = 0, alive1 = 0;
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && !ents[i].isPet) { if (ents[i].team == 0) alive0++; else alive1++; }
      if (alive0 == 0) matchResult = -1;
      if (alive1 == 0) matchResult = 1;
    } else {
      // Survivant solo/duo : victoire si tous les ennemis sont morts ET qu'il
      // reste quelqu'un de ton equipe (toi OU ton coequipier), defaite sinon
      bool myTeam = false, otherTeam = false;
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && !ents[i].isPet) { if (ents[i].team == 0) myTeam = true; else otherTeam = true; }
      if (!myTeam) matchResult = -1;        // equipe du joueur eliminee
      else if (!otherTeam) matchResult = 1; // tous les ennemis morts : on gagne
    }
  }

  if (matchTimeMs > 150000) {
    if (mode == MODE_GEM) matchResult = (teamGems[0] > teamGems[1]) ? 1 : (teamGems[0] < teamGems[1] ? -1 : 0);
    else if (mode == MODE_BOUNTY) matchResult = (teamStars[0] > teamStars[1]) ? 1 : (teamStars[0] < teamStars[1] ? -1 : 0);
    else if (mode == MODE_HEIST) matchResult = (safes[1].hp < safes[0].hp) ? 1 : ((safes[1].hp > safes[0].hp) ? -1 : 0);
    else if (mode == MODE_ZONE) {
      float m = 0, n = 0;
      for (int z = 0; z < MAX_ZONE; z++) { if (zones[z].prog > 0) m += zones[z].prog; else n -= zones[z].prog; }
      matchResult = (m > n) ? 1 : ((m < n) ? -1 : 0);
    }
    else if (!ffa && mode != MODE_KO) matchResult = 0; // pas de limite de temps en solo/duo/KO : le poison decide
  }
}

// ------------------------- Texte / polices ---------------------------
static const uint8_t FONT5[10][5] = {
  {0x0E,0x11,0x11,0x11,0x0E},{0x04,0x0C,0x04,0x04,0x0E},{0x0E,0x01,0x0E,0x10,0x0E},
  {0x0E,0x01,0x06,0x01,0x0E},{0x11,0x11,0x0E,0x01,0x01},{0x0E,0x10,0x0E,0x01,0x0E},
  {0x0E,0x10,0x0E,0x11,0x0E},{0x0E,0x01,0x02,0x04,0x04},{0x0E,0x11,0x0E,0x11,0x0E},
  {0x0E,0x11,0x0E,0x01,0x0E},
};
static const uint8_t FONTA[26][5] = {
  {0x0E,0x11,0x1F,0x11,0x11},{0x1E,0x11,0x1E,0x11,0x1E},{0x0F,0x10,0x10,0x10,0x0F},
  {0x1E,0x11,0x11,0x11,0x1E},{0x1F,0x10,0x1E,0x10,0x1F},{0x1F,0x10,0x1E,0x10,0x10},
  {0x0F,0x10,0x13,0x11,0x0F},{0x11,0x11,0x1F,0x11,0x11},{0x0E,0x04,0x04,0x04,0x0E},
  {0x07,0x02,0x02,0x02,0x1C},{0x11,0x12,0x1C,0x12,0x11},{0x10,0x10,0x10,0x10,0x1F},
  {0x11,0x1B,0x15,0x11,0x11},{0x11,0x19,0x15,0x13,0x11},{0x0E,0x11,0x11,0x11,0x0E},
  {0x1E,0x11,0x1E,0x10,0x10},{0x0E,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x1E,0x12,0x11},
  {0x0F,0x10,0x0E,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04},{0x11,0x11,0x11,0x11,0x0E},
  {0x11,0x11,0x11,0x0A,0x04},{0x11,0x11,0x15,0x1B,0x11},{0x11,0x0A,0x04,0x0A,0x11},
  {0x11,0x11,0x0F,0x01,0x0E},{0x1F,0x01,0x0E,0x10,0x1F},
};
// Rendu d'une ligne de glyphe en runs (ombre + trait), ~10 pushes par caractere
// au lieu de 30-60 : c'est le plus gros gain de performances du jeu.
// Texte : chaque bit du glyphe 5x5 est dessine en bloc 2x2 => police 10px,
// beaucoup plus lisible sur l'ecran de la calculatrice.
static void drawGlyphRow(int x, int y, uint8_t row, int ncols, KDColor c) {
  if (!row) return;
  for (int col = 0; col < ncols; col++)
    if (row & (0x10 >> col)) fRect(x + col * 2, y, 2, 2, c);
}
static void drawDigit(int x, int y, int d, KDColor c) {
  for (int r = 0; r < 5; r++) drawGlyphRow(x, y + r * 2, FONT5[d][r], 5, c); // 5 colonnes : chiffre complet
}
static void drawNum(int x, int y, int v, KDColor c) {
  if (v < 0) { fRect(x, y + 4, 6, 2, c); drawNum(x + 10, y, -v, c); return; }
  char buf[8]; int n = 0;
  do { buf[n++] = '0' + v % 10; v /= 10; } while (v);
  for (int i = n - 1; i >= 0; i--) { drawDigit(x, y, buf[i] - '0', c); x += 12; }
}
static void drawText(int x, int y, const char *s, KDColor c) {
  while (*s) {
    char ch = *s++;
    if (ch == ' ') { x += 10; continue; }
    if (ch == '-') { fRect(x, y + 4, 8, 2, c); x += 10; continue; }
    if (ch == '!') { fRect(x, y, 2, 10, c); x += 8; continue; }
    if (ch == ':') { fRect(x, y, 2, 2, c); fRect(x, y + 6, 2, 2, c); x += 8; continue; }
    if (ch == '.') { fRect(x, y + 8, 2, 2, c); x += 8; continue; }
    if (ch == '+') { fRect(x, y + 4, 8, 2, c); fRect(x + 3, y + 2, 2, 6, c); x += 12; continue; }
    if (ch >= 'A' && ch <= 'Z') {
      for (int r = 0; r < 5; r++) drawGlyphRow(x, y + r * 2, FONTA[ch - 'A'][r], 5, c);
    } else if (ch >= '0' && ch <= '9') drawDigit(x, y, ch - '0', c);
    x += 12;
  }
}
static int textW(const char *s) { int w = 0; for (const char *p = s; *p; p++) w += (*p == ' ') ? 10 : 12; return w; }
static void drawTextC(int y, const char *s, KDColor c) { drawText((SW - textW(s)) / 2, y, s, c); }

// ------------------------- Rendu du monde (themes) --------------------
static void drawWallThemed(int sx, int sy) {
  const MapTheme &T = THEMES[mode];
  KDColor w = KDColor::RGB24(T.wall), wd = KDColor::RGB24(T.wallD), wl = KDColor::RGB24(T.wallL);
  int lift = 3;
  fRect(sx, sy, TILE, TILE, wl);
  switch (T.wallStyle) {
    case 0: // brique
      fRect(sx, sy + lift, TILE, TILE - lift, wd);
      for (int row = 0; row < 2; row++)
        for (int c2 = 0; c2 < 2; c2++)
          fRect(sx + 1 + c2 * 6 + (row % 2) * 2, sy + lift + 1 + row * 4, 5, 3, w);
      break;
    case 1: // pierre taille
      fRect(sx, sy + lift, TILE, TILE - lift, wd);
      fRect(sx + 1, sy + lift + 1, TILE - 2, TILE - lift - 2, w);
      fRect(sx + 2, sy + lift + 2, TILE - 4, 2, wl);
      px(sx + 4, sy + lift + 7, wd); px(sx + 9, sy + lift + 9, wd);
      break;
    case 2: // sacs de sable
      for (int row = 0; row < 3; row++)
        for (int c2 = 0; c2 < 2; c2++) {
          int ox = sx + 1 + c2 * 6 + (row % 2) * 2;
          oCirc(ox + 2, sy + lift + 2 + row * 3, 2, w, wd);
        }
      break;
    case 3: // crates bois
      fRect(sx, sy + lift, TILE, TILE - lift, w);
      fRect(sx, sy + lift, TILE, 2, wl);
      fRect(sx, sy + lift + 3, 2, TILE - lift - 3, wl);
      fRect(sx + TILE - 3, sy + lift + 3, 2, TILE - lift - 3, wl);
      fRect(sx + 2, sy + lift + 5, TILE - 4, 2, wd);
      fRect(sx + 5, sy + lift, 2, TILE - lift, wd);
      break;
    default: // glace
      fRect(sx, sy + lift, TILE, TILE - lift, w);
      fRect(sx + 2, sy + lift + 2, 4, 3, wl);
      fRect(sx + 8, sy + lift + 6, 4, 2, wl);
      px(sx + 10, sy + lift + 2, wl);
      break;
  }
  fRect(sx, sy, TILE, 1, C_BLK); fRect(sx, sy + TILE - 1, TILE, 1, C_BLK);
  fRect(sx, sy, 1, TILE, C_BLK); fRect(sx + TILE - 1, sy, 1, TILE, C_BLK);
}
static void drawBushThemed(int sx, int sy) {
  const MapTheme &T = THEMES[mode];
  KDColor b1 = KDColor::RGB24(T.bush1), b2 = KDColor::RGB24(T.bush2);
  fRect(sx, sy, TILE, TILE, b1);
  fCirc(sx + 4, sy + 4, 4, b2);
  fCirc(sx + 10, sy + 5, 4, b2);
  fCirc(sx + 7, sy + 10, 4, b2);
  px(sx + 5, sy + 3, KDColor::RGB24(T.grDecor)); px(sx + 11, sy + 4, KDColor::RGB24(T.grDecor));
}
static void drawGroundDecor(int sx, int sy, int tx, int ty) {
  const MapTheme &T = THEMES[mode];
  KDColor dec = KDColor::RGB24(T.grDecor);
  switch (T.decoStyle) {
    case 0: px(sx + (tx * 5 + ty * 3) % 12, sy + (tx * 3 + ty * 7) % 12, dec); break;
    case 1: // fleurs
      if (((tx * 7 + ty * 13) & 7) == 0) {
        int bx = (tx * 5 + ty * 3) % 10, by = (tx * 3 + ty * 7) % 10;
        px(sx + bx, sy + by, dec); px(sx + bx - 1, sy + by, C_MAG); px(sx + bx + 1, sy + by, C_MAG);
        px(sx + bx, sy + by - 1, C_MAG); px(sx + bx, sy + by + 1, C_MAG);
      }
      break;
    case 2: // cranes / pierres
      if (((tx * 7 + ty * 13) & 15) == 0) {
        int bx = (tx * 3 + ty * 5) % 10, by = (tx * 5 + ty * 3) % 10;
        fRect(sx + bx, sy + by, 3, 2, KDColor::RGB24(0xE0D8C0));
        px(sx + bx, sy + by, C_BLK); px(sx + bx + 2, sy + by, C_BLK);
      }
      break;
    case 3: // lignes de stade
      if (ty % 4 == 2) fRect(sx, sy + 6, TILE, 1, dec);
      break;
    default: // neige / eclats de glace
      if (((tx * 7 + ty * 13) & 7) == 0) {
        int bx = (tx * 5 + ty * 3) % 12, by = (tx * 3 + ty * 7) % 12;
        fRect(sx + bx, sy + by, 2, 1, dec); px(sx + bx + 2, sy + by + 1, dec);
      }
      break;
  }
}
static void drawGem(int cx, int cy, int t) {
  int bob = (t / 400) % 2;
  shadow(cx, cy + 5, 4, 2);
  for (int j = -5; j <= 5; j++)
    for (int i = -4; i <= 4; i++) {
      int w = (j < 0) ? 4 + j : 4 - j;
      if (abs(i) <= w) px(cx + i, cy + j - bob, (i < -1 || j < -2) ? C_GEMD : C_GEM);
    }
  px(cx - 1, cy - 3, C_WHT); px(cx, cy - 4, C_WHT);
}
static void drawBall(int cx, int cy) {
  shadow(cx, cy + 6, 5, 2);
  oCirc(cx, cy, 5, C_BALL, C_BLK);
  fRect(cx - 5, cy - 1, 10, 2, C_BLK);
  px(cx - 2, cy - 3, C_WHT);
}
static void drawHPBar(int sx, int sy, float hp, float maxHP, KDColor col, KDColor colD) {
  int w = 18;
  fRect(sx - w / 2 - 1, sy - 1, w + 2, 6, C_BLK);
  fRect(sx - w / 2, sy, w, 4, KDColor::RGB24(0x3A3A3A));
  int fill = (int)(w * hp / maxHP);
  fRect(sx - w / 2, sy, fill, 4, col);
  fRect(sx - w / 2, sy + 3, fill, 1, colD);
  for (int s = 4; s < w; s += 4) fRect(sx - w / 2 + s, sy, 1, 4, C_BLK);
}
static void drawEnt(Ent &e, int sx, int sy) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  int myTeam = ents[humanIdx].team;
  bool teamRed = (e.team != myTeam);
  KDColor tCol = teamRed ? C_RED : C_BLUE;
  KDColor tColD = teamRed ? C_REDD : C_BLUED;
  bool flash = e.lastHitT > 0 && (e.lastHitT / 50) % 2 == 0;
  int bob = ((e.walkT / 40) % 2) * (e.dashT > 0 ? 2 : 1);
  int by = sy - bob;

  if (e.isPet) { // rendu des creatures invoquees
    shadow(sx, sy + 6, 6, 3);
    if (e.petKind == 0) { // ours de Nita
      fCirc(sx - 5, by - 9, 3, tColD); fCirc(sx + 5, by - 9, 3, tColD);
      fCirc(sx, by - 4, 8, flash ? C_WHT : tCol); fCirc(sx, by - 2, 8, tColD);
      fCirc(sx - 3, by - 6, 1, C_BLK); fCirc(sx + 3, by - 6, 1, C_BLK);
      fRect(sx - 2, by - 2, 4, 2, C_BLK);
    } else if (e.petKind == 1) { // tourelle (Jessie, Penny, 8-Bit)
      fRect(sx - 7, by, 14, 5, tColD);
      fRect(sx - 5, by - 6, 10, 7, flash ? C_WHT : tCol);
      fRect(sx + (int)(cosf(e.aimAng) * 5) - 1, by - 3 + (int)(sinf(e.aimAng) * 4), 5, 2, C_BLK);
      fCirc(sx - 3, by - 4, 1, C_YEL); fCirc(sx + 3, by - 4, 1, C_YEL);
    } else if (e.petKind == 2) { // station de soin de Pam
      int r = 7 + (animT / 200) % 2;
      fCirc(sx, by - 2, r, flash ? C_WHT : C_GREEN);
      fRect(sx - 1, by - 6, 2, 8, C_WHT); fRect(sx - 4, by - 3, 8, 2, C_WHT);
      fRing(sx, by - 2, r + 2 + (animT / 300) % 3, C_GREEN);
    } else if (e.petKind == 3) { // rats d'Ash
      fCirc(sx, by - 3, 5, flash ? C_WHT : C_GRE);
      fRect(sx + 3, by - 1, 4, 1, C_GRE); // queue
      fCirc(sx + 2, by - 5, 1, C_RED); fCirc(sx - 1, by - 5, 1, C_RED);
    } else if (e.petKind == 4) { // ego de Lola : mini clone aux couleurs du proprio
      drawBrawlerSprite(sx, by, 5, e.brawlerIdx, (int)e.faceAng, flash, e.walkT, e.attackT);
      fRing(sx, by, 7, tCol);
    } else { // pingouins de Mr. P
      fCirc(sx, by - 3, 6, flash ? C_WHT : tCol);
      fRect(sx - 2, by - 2, 4, 4, C_WHT); // ventre
      fCirc(sx + 1, by - 6, 1, C_BLK); fCirc(sx - 2, by - 6, 1, C_BLK);
    }
    if (e.petLife < 5 && (e.petLife * 2) - (int)(e.petLife * 2) < 0.5f) return; // clignote avant de disparaitre
    drawHPBar(sx, by - 16, e.hp, e.maxHP, e.team == ents[humanIdx].team ? C_GREEN : tCol, tColD);
    return;
  }

  // invisible (super de Leon / Sandy) : juste un scintillement
  if (e.stealthT > 0) {
    shadow(sx, sy + 7, 7, 3);
    if ((animT / 160) % 2 == 0) { fCirc(sx, sy - 2, 2, tCol); fCirc(sx + 5, sy - 5, 1, tCol); }
    return;
  }

  shadow(sx, sy + 7, 7, 3);
  if (e.hyperT > 0) { fRing(sx, sy, 12, C_MAG); fRing(sx, sy, 13 + (animT / 80) % 2, C_PUR); }
  drawBrawlerSprite(sx, by, 7, e.brawlerIdx, (int)e.faceAng, flash, e.walkT, e.attackT);
  arc(sx, by, 8, 3.34f, 6.08f, 2, tCol);
  int gx = (int)(cosf(e.aimAng) * 9), gy = (int)(sinf(e.aimAng) * 9);
  fCirc(sx + gx, by + gy, 2, C_GRE);
  for (int a = 0; a < 3; a++) {
    int filled = (int)(fmaxf(0, fminf(1, e.ammo - a)) * 4);
    fRect(sx - 8 + a * 6, by + 9, 5, 2, C_BLK);
    fRect(sx - 8 + a * 6, by + 9, filled, 2, C_ORG);
  }
  if (e.superCharge >= 100) {
    int r = 10 + (animT / 150) % 2;
    fRing(sx, by, r, C_PUR);
    fCirc(sx + (int)(cosf(animT * 0.01f) * r), by + (int)(sinf(animT * 0.01f) * r), 1, C_YEL);
  } else if (e.superCharge > 0) {
    arc(sx, by, 10, -1.5708f, -1.5708f + 6.2832f * e.superCharge / 100.0f, 2, C_PUR);
  }
  drawHPBar(sx, by - 15, e.hp, e.maxHP, e.isHuman ? C_GREEN : tCol, tColD);
  if (e.gems > 0) {
    fCirc(sx + 12, by - 12, 6, C_GREEN);
    drawNum(sx + 9, by - 14, e.gems, C_WHT);
  }
  if (mode == MODE_BOUNTY && e.stars > 0) {
    fCirc(sx - 12, by - 12, 6, C_GOLD);
    drawNum(sx - 15, by - 14, e.stars, C_BLK);
  }
  if (e.hasBall) oCirc(sx, by - 22, 4, C_BALL, C_BLK);
}
static void drawProj(Proj &p, int sx, int sy, int myTeam) {
  KDColor c = p.fromSuper ? C_PUR : (p.team == myTeam ? C_YEL : C_RED);
  KDColor c2 = p.fromSuper ? C_MAG : C_ORG;
  switch (p.shape) {
    case 0: oCirc(sx, sy, p.radius + 1, c, C_BLK); break;
    case 1:
      for (int t = -2; t <= 2; t++)
        px(sx + (int)(p.vx * t * 0.008f), sy + (int)(p.vy * t * 0.008f), t < 0 ? c : c2);
      break;
    case 2:
      oCirc(sx, sy, p.radius + 1, c, C_BLK);
      px(sx - (int)(p.vx * 0.02f), sy - (int)(p.vy * 0.02f), C_ORG);
      px(sx - (int)(p.vx * 0.03f), sy - (int)(p.vy * 0.03f), C_YEL);
      break;
    default:
      fCirc(sx, sy, p.radius + 1, c);
      px(sx + (p.spin / 8) % 3 - 1, sy - (p.spin / 16) % 2, C_BLK);
      break;
  }
}

static void renderMatch() {
  const MapTheme &T = THEMES[mode];
  Ent &me = ents[humanIdx];
  int camX = (int)me.x - SW / 2, camY = (int)me.y - SH / 2;
  camX = fmaxf(0, fminf(MAPW * TILE - SW, camX));
  camY = fmaxf(0, fminf(MAPH * TILE - SH, camY));
  if (shakeT > 0) { camX += rnd(shakeMag * 2) - shakeMag; camY += rnd(shakeMag * 2) - shakeMag; }

  for (int ty = camY / TILE; ty <= (camY + SH) / TILE && ty < MAPH; ty++) {
    int sy = ty * TILE - camY;
    // perf : fond en 2 pushes par ligne de tuiles au lieu de 23
    fRect(0, sy, SW, TILE, KDColor::RGB24(T.gr1));
    for (int tx = camX / TILE; tx <= (camX + SW) / TILE && tx < MAPW; tx++)
      if ((tx + ty) & 1) {
        int sx = tx * TILE - camX;
        fRect(sx, sy, TILE, TILE, KDColor::RGB24(T.gr2));
      }
    for (int tx = camX / TILE; tx <= (camX + SW) / TILE && tx < MAPW; tx++) {
      int sx = tx * TILE - camX;
      drawGroundDecor(sx, sy, tx, ty);
      if (tileMap[tx][ty] == 1) drawWallThemed(sx, sy);
      else if (tileMap[tx][ty] == 2) drawBushThemed(sx, sy);
    }
  }

  if (mode == MODE_BALL)
    for (int ty = MAPH / 2 - 2; ty <= MAPH / 2 + 2; ty++) {
      fRect(2 - camX, ty * TILE - camY + 2, TILE - 4, TILE - 4, C_BLUED);
      fRect(MAPW * TILE - TILE - camX + 2, ty * TILE - camY + 2, TILE - 4, TILE - 4, C_REDD);
    }

  if (mode == MODE_HEIST)
    for (int s = 0; s < MAX_SAFE; s++) {
      if (safes[s].maxHP <= 0) continue;
      int sx = (int)safes[s].x - camX, sy = (int)safes[s].y - camY;
      shadow(sx, sy + 14, 14, 4);
      fRect(sx - 13, sy - 13, 26, 26, safes[s].team == 0 ? C_BLUED : C_REDD);
      fRect(sx - 11, sy - 11, 22, 22, safes[s].team == 0 ? C_BLUE : C_RED);
      fRect(sx - 4, sy - 3, 8, 6, C_YEL);
      fRect(sx - 12, sy - 18, 24, 5, C_BLK);
      fRect(sx - 11, sy - 17, (int)(22 * safes[s].hp / safes[s].maxHP), 3, C_GOLD);
    }

  if (mode == MODE_ZONE)
    for (int z = 0; z < MAX_ZONE; z++) {
      int sx = (int)zones[z].x - camX, sy = (int)zones[z].y - camY;
      KDColor zc = zones[z].prog >= 0 ? (zones[z].prog > 66 ? C_GREEN : C_BLUE) : C_RED;
      fRing(sx, sy, 24, zc); fRing(sx, sy, 25, C_BLK);
      if (fabsf(zones[z].prog) > 3) {
        fRect(sx - 15, sy - 3, 30, 7, C_BLK);
        int fw = (int)(28 * fabsf(zones[z].prog) / 100.0f);
        fRect(sx - 14, sy - 2, fw, 5, zones[z].prog >= 0 ? C_GREEN : C_RED);
      }
    }

  if (mode == MODE_SOLO || mode == MODE_DUO || mode == MODE_KO) {
    int cx = MAPW * TILE / 2 - camX, cy = MAPH * TILE / 2 - camY;
    for (int a = 0; a < 360; a += 6) {
      int ex = cx + (int)(soloShrink * cosf(a * 0.01745f));
      int ey = cy + (int)(soloShrink * sinf(a * 0.01745f));
      fRect(ex - 2, ey - 2, 5, 5, C_POISON);
      if ((a / 6 + animT / 200) % 4 == 0) fRect(ex - 3, ey - 3, 7, 7, C_POIS2);
    }
  }

  if (mode == MODE_GEM)
    for (int g = 0; g < 10; g++)
      if (gems[g].active) drawGem((int)gems[g].x - camX, (int)gems[g].y - camY, animT + g * 300);
  if (mode == MODE_BALL) drawBall((int)ballX - camX, (int)ballY - camY);

  for (int pass = 0; pass < entCount; pass++) {
    int best = -1, bestY = 1 << 30;
    for (int i = 0; i < entCount; i++) {
      Ent &e = ents[i];
      if (!e.alive || e.drawDone) continue;
      if ((int)e.y < bestY) { bestY = (int)e.y; best = i; }
    }
    if (best < 0) break;
    ents[best].drawDone = true;
    Ent &e = ents[best];
    int sx = (int)e.x - camX, sy2 = (int)e.y - camY;
    if (best != humanIdx && inBush(e.x, e.y)) {
      float dx = e.x - me.x, dy = e.y - me.y;
      if (dx * dx + dy * dy > 60 * 60) continue;
    }
    drawEnt(e, sx, sy2);
  }
  for (int i = 0; i < entCount; i++) ents[i].drawDone = false;

  for (int i = 0; i < MAX_PROJ; i++)
    if (projs[i].active) {
      shadow((int)projs[i].x - camX, (int)projs[i].y - camY + 6, 2, 1);
      drawProj(projs[i], (int)projs[i].x - camX, (int)projs[i].y - camY, ents[humanIdx].team);
    }

  for (int i = 0; i < MAX_POP; i++)
    if (pops[i].active)
      drawNum((int)pops[i].x - camX - 6, (int)pops[i].y - camY, pops[i].val, C_WHT);

  // particules
  for (int i = 0; i < MAX_PART; i++)
    if (parts[i].active) {
      int sx = (int)parts[i].x - camX, sy = (int)parts[i].y - camY;
      if (parts[i].kind == 0) { px(sx, sy, C_WHT); px(sx + 1, sy, C_YEL); }
      else { px(sx, sy, C_ORG); px(sx - 1, sy + 1, C_RED); px(sx + 1, sy - 1, C_YEL); }
    }

  // killfeed (coin haut droit, sous le HUD)
  for (int i = 0; i < KILLFEED; i++)
    if (killfeed[i].alive) {
      int y = 28 + i * 14;
      fRect(SW - 118, y, 118, 12, C_BLK);
      drawBrawlerSprite(SW - 110, y + 6, 4, killfeed[i].killer, 90, false);
      drawText(SW - 102, y + 1, "A TUE", C_GRE);
      drawBrawlerSprite(SW - 32, y + 6, 4, killfeed[i].victim, 90, false);
    }

  // ---- Minimap (coin bas droit) ----
  {
    int mw = 64, mh = 48, mx = SW - mw - 2, my = SH - mh - 14;
    fRect(mx - 1, my - 1, mw + 2, mh + 2, C_BLK);
    fRect(mx, my, mw, mh, KDColor::RGB24(0x1E2A38));
    // murs en runs (1 push par ligne au lieu de ~200 pushes par pixel)
    for (int ty = 0; ty < MAPH; ty += 2) {
      rowClear(mx, mx + mw - 1);
      for (int tx = 0; tx < MAPW; tx += 2)
        if (tileMap[tx][ty] == 1) rowPut(mx + tx * mw / MAPW, C_GRE);
      rowFlush(my + ty * mh / MAPH, mx, mx + mw - 1);
    }
    if (mode == MODE_GEM)
      for (int g = 0; g < 10; g++)
        if (gems[g].active) px(mx + (int)(gems[g].x * mw / (MAPW * TILE)), my + (int)(gems[g].y * mh / (MAPH * TILE)), C_GEM);
    if (mode == MODE_BALL) px(mx + (int)(ballX * mw / (MAPW * TILE)), my + (int)(ballY * mh / (MAPH * TILE)), C_WHT);
    if (mode == MODE_HEIST)
      for (int s = 0; s < MAX_SAFE; s++)
        if (safes[s].maxHP > 0) px(mx + (int)(safes[s].x * mw / (MAPW * TILE)), my + (int)(safes[s].y * mh / (MAPH * TILE)), C_GOLD);
    for (int i = 0; i < entCount; i++)
      if (ents[i].alive) {
        KDColor c = (i == humanIdx) ? C_GREEN : (ents[i].team == me.team ? C_BLUE : C_RED);
        px(mx + (int)(ents[i].x * mw / (MAPW * TILE)), my + (int)(ents[i].y * mh / (MAPH * TILE)), c);
        if (i == humanIdx) { px(mx + (int)(ents[i].x * mw / (MAPW * TILE)) + 1, my + (int)(ents[i].y * mh / (MAPH * TILE)), c); }
      }
  }

  // ---- HUD (texte 10px : tout tient sur une barre de 26px) ----
  fRect(0, 0, SW, 26, C_PANEL);
  fRect(0, 24, SW, 2, C_PANEL2);
  const BrawlerDef &d = BRAWLERS[me.brawlerIdx];
  drawBrawlerSprite(14, 13, 8, me.brawlerIdx, 90, false);
  drawText(28, 3, d.name, C_WHT);
  int lvl = (me.accIdx >= 0) ? acc.b[me.accIdx].level : 1;
  fRect(32 + textW(d.name), 1, 24, 20, C_YEL);
  fRect(32 + textW(d.name), 15, 24, 6, C_BTND);
  drawNum(36 + textW(d.name), 3, lvl, C_BLK);
  // chrono du match
  drawNum(150, 3, fmaxf(0, 150 - matchTimeMs / 1000), C_WHT);
  // indicateur de l'action armee (OK = super, X = gadget) + hypercharge
  if (mode == MODE_GEM && gemCountdown > 0) {
    drawText(190, 3, "FIN!", C_YEL);
    drawNum(244, 3, gemCountdown / 1000 + 1, C_YEL);
  } else if (me.hyperT > 0) { drawText(190, 3, "HYP!", C_MAG); }
  else if (me.superCharge >= 100) { drawText(190, 3, "ULTI!", C_YEL); } // ulti prete (touche RETOUR)
  // gadget (SHIFT) : nombre d'utilisations restantes
  fRect(268, 1, 48, 24, me.gadgetUses > 0 ? C_CYN : KDColor::RGB24(0x225566));
  drawText(274, 3, "SH", C_BLK);
  drawNum(288, 3, me.gadgetUses, C_WHT);

  // compte a rebours 3-2-1-GO au lancement du match
  if (startCountdown > -800) {
    int n = (startCountdown + 999) / 1000;
    fRect(SW / 2 - 60, 84, 120, 60, C_BLK);
    fRect(SW / 2 - 58, 86, 116, 56, C_PANEL);
    if (n > 0) drawNum(SW / 2 - 12, 104, n, C_YEL);
    else drawTextC(104, "GO!", C_GREEN);
  }
  // ecran de reaparition du joueur
  if (!me.alive) {
    fRect(SW / 2 - 96, 90, 192, 44, C_BLK);
    fRect(SW / 2 - 94, 92, 188, 40, C_PANEL);
    drawTextC(96, "REAPPARITION", C_WHT);
    drawNum(SW / 2 - 12, 114, (me.respawnT + 999) / 1000, C_YEL);
  }
  // bandeau bas : rappel des touches a gauche, score du mode a droite
  fRect(0, SH - 14, SW, 14, KDColor::RGB24(0x111827));
  drawText(4, SH - 12, "OK:ATK RET:ULTI SH:GAD", C_GRE);
  if (mode == MODE_GEM) {
    drawNum(SW - 72, SH - 12, teamGems[0], C_BLUE);
    drawText(SW - 48, SH - 12, "-", C_WHT);
    drawNum(SW - 38, SH - 12, teamGems[1], C_RED);
  } else if (mode == MODE_BALL) {
    int g0 = 0, g1 = 0;
    for (int i = 0; i < entCount; i++)
      if (ents[i].team == 0) g0 = fmaxf(g0, ents[i].goals);
      else g1 = fmaxf(g1, ents[i].goals);
    drawNum(SW - 72, SH - 12, g0, C_BLUE);
    drawText(SW - 48, SH - 12, "-", C_WHT);
    drawNum(SW - 38, SH - 12, g1, C_RED);
  } else if (mode == MODE_BOUNTY) {
    drawNum(SW - 72, SH - 12, teamStars[0], C_GOLD);
    drawText(SW - 48, SH - 12, "-", C_WHT);
    drawNum(SW - 38, SH - 12, teamStars[1], C_RED);
  } else if (mode == MODE_HEIST) {
    drawText(SW - 72, SH - 12, "S", C_GRE);
    drawNum(SW - 60, SH - 12, (int)(safes[0].hp / 1000), C_BLUE);
  } else if (mode == MODE_ZONE) {
    int m = 0;
    for (int z = 0; z < MAX_ZONE; z++) if (zones[z].prog > 0) m++;
    drawText(SW - 72, SH - 12, "Z", C_WHT);
    drawNum(SW - 60, SH - 12, m, C_GREEN);
  } else if (mode == MODE_KO) {
    int a0 = 0, a1 = 0;
    for (int i = 0; i < entCount; i++)
      if (ents[i].alive) { if (ents[i].team == 0) a0++; else a1++; }
    drawNum(SW - 72, SH - 12, a0, C_BLUE);
    drawText(SW - 48, SH - 12, "-", C_WHT);
    drawNum(SW - 38, SH - 12, a1, C_RED);
  } else {
    int na = 0;
    for (int i = 0; i < entCount; i++) if (ents[i].alive) na++;
    drawText(SW - 72, SH - 12, "V", C_WHT);
    drawNum(SW - 60, SH - 12, na, C_YEL);
  }
  blit();
}

// ------------------------- Quetes -------------------------------------
// 3 quetes actives, remplacees quand terminees. Repondees en jeu.
// (structure Quest et compteurs declares plus haut, pres du killfeed)

static void rollQuest(Quest &q) {
  int t = rnd(5);
  q.type = t;
  switch (t) {
    case 0: q.goal = 1 + rnd(2); q.reward = 80; break;   // matchs gagnes
    case 1: q.goal = 3 + rnd(5); q.reward = 60; break;   // eliminations
    case 2: q.goal = 5 + rnd(6); q.reward = 50; break;   // gemmes
    case 3: q.goal = 3 + rnd(4); q.reward = 40; break;   // supers
    default: q.goal = 2 + rnd(2); q.reward = 120; break;  // serie de victoires
  }
  q.prog = 0;
}
static const char* questDesc(Quest &q, char *buf) {
  const char *base;
  switch (q.type) {
    case 0: base = "GAGNE %d MATCHS"; break;
    case 1: base = "%d ELIMINATIONS"; break;
    case 2: base = "RAMASSE %d GEMMES"; break;
    case 3: base = "UTILISE %d SUPERS"; break;
    default: base = "%d VICTOIRES DE SUITE"; break;
  }
  (void)base;
  // formatage manuel simple
  int g = q.goal, pos = 0;
  const char *prefix[] = {"GAGNE ", "ELIMINE ", "RAMASSE ", "SUPER ", "VICTOIRES "};
  const char *suffix[] = {" MATCHS", " FOIS", " GEMMES", " FOIS", " DE SUITE"};
  const char *p = prefix[q.type], *s = suffix[q.type];
  while (*p) buf[pos++] = *p++;
  char tmp[8]; int tn = 0;
  do { tmp[tn++] = '0' + g % 10; g /= 10; } while (g);
  while (tn > 0) buf[pos++] = tmp[--tn];
  while (*s) buf[pos++] = *s++;
  buf[pos] = 0;
  return buf;
}
static void initQuests() {
  killCounter = gemCounter = superCounter = winStreak = 0;
  for (int i = 0; i < 3; i++) rollQuest(quests[i]);
}
static void questScreen() {
  waitRelease();
  { // dessin unique (ecran statique, pas de redraw a 60fps)
    bgPanel();
    drawTextC(8, "QUETES DU JOUR", C_YEL);
    fRect(SW / 2 - 60, 22, 120, 3, C_BTND);
    for (int i = 0; i < 3; i++) {
      Quest &q = quests[i];
      int y = 44 + i * 52;
      bool done = q.prog >= q.goal;
      rRect(30, y, SW - 60, 42, 8, done ? C_GREEN : KDColor::RGB24(0x2B3A55));
      char buf[32];
      drawText(40, y + 6, questDesc(q, buf), done ? C_BLK : C_WHT);
      // barre de progression
      fRect(40, y + 20, 180, 8, C_BLK);
      fRect(41, y + 21, (int)(178 * fminf(1.0f, (float)q.prog / q.goal)), 6, done ? C_GOLD : C_CYN);
      drawNum(226, y + 18, q.prog, C_WHT);
      drawText(254, y + 18, "SUR", C_GRE);
      drawNum(294, y + 18, q.goal, C_WHT);
      // recompense
      oCirc(46, y + 36, 4, C_YEL, C_BTND);
      drawNum(54, y + 32, q.reward, done ? C_BLK : C_YEL);
      if (done) drawText(80, y + 32, "COMPLETEE!", C_BLK);
    }
    drawTextC(SH - 14, "OK RETOUR", C_GRE);
    blit();
  }
  while (true) {
    pollKeys();
    if (keyOK && !prevOK) { prevOK = true; return; }
    prevOK = keyOK;
    Ion::Timing::msleep(60);
  }
}
static void updateQuests(int result) {
  // appelee a la fin de chaque match
  if (result == 1) {
    winStreak++;
  } else if (result < 0) {
    winStreak = 0;
  }
  for (int i = 0; i < 3; i++) {
    Quest &q = quests[i];
    bool wasDone = q.prog >= q.goal;
    switch (q.type) {
      case 0: if (result == 1) q.prog++; break;
      case 1: q.prog = killCounter; break;
      case 2: q.prog = gemCounter; break;
      case 3: q.prog = superCounter; break;
      default: q.prog = (winStreak >= q.goal) ? q.goal : winStreak; break;
    }
    if (!wasDone && q.prog >= q.goal) {
      acc.coins += q.reward;
      rollQuest(q); // nouvelle quete
      saveAcc(); // les quetes et pieces gagnees persistent
    }
  }
  saveAcc();
}

// ------------------------- Menus --------------------------------------
static const char **gMenuDesc = nullptr; // descriptions optionnelles des items du menu
static int menuPick(const char *title, const char **items, int n, int start) {
  int sel = start;
  bool pU = false, pD = false;
  int drawn = -1;
  waitRelease();
  while (true) {
    if (drawn != sel) { // redessine seulement quand la selection bouge (perf)
      bgPanel();
      drawTextC(6, title, C_YEL);
      fRect(SW / 2 - 60, 20, 120, 3, C_BTND);
      int first = (sel > 4) ? sel - 4 : 0;
      if (first > n - 9) first = fmaxf(0, n - 9);
      for (int i = first; i < n && i < first + 9; i++) {
        int y = 28 + (i - first) * 19;
        if (i == sel) rRect(36, y, SW - 72, 16, 6, C_YEL);
        else rRect(36, y, SW - 72, 16, 6, C_BTN);
        drawTextC(y + 3, items[i], i == sel ? C_BLK : C_WHT);
      }
      if (gMenuDesc && gMenuDesc[sel]) drawTextC(206, gMenuDesc[sel], C_GRE); // description du mode
      drawTextC(SH - 14, "OK CHOISIR   BACK RETOUR", C_GRE);
      blit();
      drawn = sel;
    }
    pollKeys();
    if (kUp && !pU && sel > 0) sel--; // front montant seulement : 1 appui = 1 cran
    if (kDown && !pD && sel < n - 1) sel++;
    pU = kUp; pD = kDown;
    if (keyOK && !prevOK) { prevOK = true; return sel; }
    if (keyBack && !prevBack) { prevBack = true; return -1; }
    prevOK = keyOK; prevBack = keyBack;
    Ion::Timing::msleep(50);
  }
}

static void trophyRoadScreen() {
  waitRelease();
  { // dessin unique (ecran statique)
    bgPanel();
    drawTextC(6, "ROUTE DES TROPHEES", C_YEL);
    fRect(SW / 2 - 60, 20, 120, 3, C_BTND);
    int tt = totalTrophies();
    drawText(20, 30, "TOTAL:", C_WHT);
    fRect(96, 28, 10, 5, C_GOLD); fRect(98, 34, 8, 2, C_GOLD); fRect(101, 37, 3, 5, C_GOLD);
    drawNum(114, 30, tt, C_YEL);
    drawText(20, 48, "BRAWLERS:", C_WHT);
    drawNum(150, 48, unlockedBrawlers, C_GREEN);
    drawText(180, 48, "SUR 50", C_GRE);
    drawText(20, 66, "PIECES:", C_WHT);
    oCirc(110, 68, 4, C_YEL, C_BTND);
    drawNum(120, 66, acc.coins, C_YEL);

    int next = -1;
    for (int i = 0; i < 55; i++)
      if (!isUnlocked(i)) { next = i; break; }
    drawTextC(76, "PROCHAIN DEBLOCAGE", C_WHT);
    if (next >= 0) {
      const BrawlerDef &d = BRAWLERS[next];
      rRect(SW / 2 - 110, 84, 220, 80, 8, KDColor::RGB24(RAR_COL[d.rar]));
      drawBrawlerSprite(SW / 2 - 80, 122, 16, next, 90, false);
      drawText(SW / 2 - 55, 92, d.name, C_WHT);
      drawText(SW / 2 - 55, 106, RAR_NAME[d.rar], KDColor::RGB24(RAR_COL[d.rar]));
      drawText(SW / 2 - 55, 122, "A", C_WHT);
      drawNum(SW / 2 - 41, 122, d.unlockTroph, C_YEL);
      drawText(SW / 2 - 55, 138, "TROPHEES", C_GRE);
      int prog = fminf(1.0f, (float)tt / d.unlockTroph);
      fRect(SW / 2 - 55, 152, 160, 8, C_BLK);
      fRect(SW / 2 - 54, 153, (int)(158 * prog), 6, C_GREEN);
    } else {
      drawTextC(110, "TOUT EST DEBLOQUE!", C_GOLD);
    }
    drawTextC(SH - 14, "OK RETOUR", C_GRE);
    blit();
  }
  // reste affiche jusqu'a un vrai appui OK (avant : l'ecran sautait aussitot)
  while (true) {
    pollKeys();
    if (keyOK && !prevOK) { prevOK = true; return; }
    if (keyBack && !prevBack) { prevBack = true; return; }
    prevOK = keyOK; prevBack = keyBack;
    Ion::Timing::msleep(60);
  }
}

// ecran de selection de personnage : liste + grand portrait style BS
static int brawlerPicker(int myBrawler) {
  int sel = myBrawler;
  bool pU = false, pD = false;
  waitRelease();
  while (true) {
    bgPanel();
    drawTextC(6, "CHOISIS TON BRAWLER", C_YEL);
    fRect(SW / 2 - 60, 20, 120, 3, C_BTND);

    // liste a gauche (11 visibles)
    int first = (sel > 5) ? sel - 5 : 0;
    if (first > 55 - 11) first = fmaxf(0, 55 - 11);
    for (int i = first; i < 55 && i < first + 11; i++) {
      int y = 30 + (i - first) * 15;
      bool un = isUnlocked(i);
      if (i == sel) rRect(6, y - 3, 130, 14, 5, C_YEL);
      // mini-portrait
      drawBrawlerSprite(16, y + 3, 5, i, 90, false);
      drawText(26, y, BRAWLERS[i].name, i == sel ? C_BLK : (un ? C_WHT : C_GRE));
      if (!un) drawText(26 + textW(BRAWLERS[i].name) + 2, y, "?", C_GRE);
    }

    // grand portrait a droite avec cadre de rarete
    const BrawlerDef &d = BRAWLERS[sel];
    bool un = isUnlocked(sel);
    rRect(150, 30, 164, 150, 10, un ? KDColor::RGB24(RAR_COL[d.rar]) : C_DGREY);
    int bob = (animT / 400) % 2;
    if (un) {
      drawBrawlerSprite(232, 80 - bob, 30, sel, 90, false);
      shadow(232, 116, 22, 5);
      drawText(150 + (164 - textW(d.name)) / 2, 136, d.name, C_YEL);
      drawText(150 + (164 - textW(RAR_NAME[d.rar])) / 2, 150, RAR_NAME[d.rar], KDColor::RGB24(RAR_COL[d.rar]));
      // trophees du perso (centres dans le panneau de droite)
      fRect(190, 166, 10, 5, C_GOLD); fRect(192, 172, 8, 2, C_GOLD);
      drawNum(206, 165, acc.b[sel].trophies, C_WHT);
      // niveau
      fRect(240, 160, 32, 16, C_YEL);
      drawNum(248, 163, acc.b[sel].level, C_BLK);
    } else {
      shCircRGB(232, 80 - bob, 30, 0x444444, 0x222222, C_GRE);
      drawText(150 + (164 - textW("? ? ?")) / 2, 90, "? ? ?", C_GRE);
      drawText(150 + (164 - textW("A DEBLOQUER A")) / 2, 120, "A DEBLOQUER A", C_WHT);
      char buf[24];
      int n = d.unlockTroph, pos = 0;
      buf[pos++] = 'A'; buf[pos++] = ' '; buf[pos++] = ' ';
      char tmp[8]; int tn = 0;
      do { tmp[tn++] = '0' + n % 10; n /= 10; } while (n);
      while (tn > 0) buf[pos++] = tmp[--tn];
      buf[pos++] = ' '; buf[pos++] = 'T'; buf[pos++] = 'R'; buf[pos++] = 'O'; buf[pos++] = 'P'; buf[pos++] = 0;
      drawText(150 + (164 - textW(buf)) / 2, 136, buf, C_YEL);
      drawText(150 + (164 - textW("TROPHEES")) / 2, 152, "TROPHEES", C_GRE);
    }
    drawTextC(SH - 14, "OK CHOISIR   BACK RETOUR", C_GRE);
    blit();
    animT += 100;
    pollKeys();
    if (kUp && !pU && sel > 0) sel--; // front montant seulement
    if (kDown && !pD && sel < 49) sel++;
    pU = kUp; pD = kDown;
    if (keyOK && !prevOK) { prevOK = true; return sel; }
    if (keyBack && !prevBack) { prevBack = true; return -1; }
    prevOK = keyOK; prevBack = keyBack;
    Ion::Timing::msleep(60);
  }
}

// ecran des parametres : rappel des touches
static void settingsScreen() {
  waitRelease();
  { // dessin unique
    bgPanel();
    drawTextC(8, "PARAMETRES", C_YEL);
    fRect(SW / 2 - 60, 22, 120, 3, C_BTND);
    drawTextC(32, "TOUCHES DU JEU", C_CYN);
    const char *acts[7] = {"SE DEPLACER", "ATTAQUE", "ULTI", "GADGET", "HYPERCHARGE", "QUITTER", "RETOUR MENU"};
    const char *keys[7] = {"FLECHES", "OK / VAR", "RETOUR", "SHIFT", "EFFACER", "HOME", "BACK"};
    for (int i = 0; i < 7; i++) {
      int y = 40 + i * 15;
      rRect(16, y - 2, 150, 14, 4, KDColor::RGB24(0x2B3A55));
      drawText(22, y, acts[i], C_WHT);
      rRect(186, y - 2, 120, 14, 4, C_BTN);
      drawText(192, y, keys[i], C_BLK);
    }
    drawTextC(158, "+ NIV  * GADGET  / STAR", C_GRE);
    rRect(SW / 2 - 70, 190, 140, 24, 8, C_YEL);
    drawTextC(196, "OK RETOUR", C_BLK);
    blit();
  }
  while (true) {
    pollKeys();
    if (keyOK && !prevOK) { prevOK = true; return; }
    if (keyBack && !prevBack) { prevBack = true; return; }
    prevOK = keyOK; prevBack = keyBack;
    Ion::Timing::msleep(60);
  }
}

static void brawlerMenu(int myBrawler) {
  while (true) {
    int sel = brawlerPicker(myBrawler);
    if (sel < 0) return;
    myBrawler = sel;
    waitRelease(); // sinon le OK du picker refermait la fiche aussitot
    while (true) {
      const BrawlerDef &d = BRAWLERS[sel];
      BrawlerAccount &a = acc.b[sel];
      bool unlocked = isUnlocked(sel);
      float mul = lvlMul(a.level);
      int cost = upgradeCost(a.level);
      bgPanel();

      int bob = (animT / 400) % 2;
      rRect(18, 26, 62, 70, 8, unlocked ? KDColor::RGB24(RAR_COL[d.rar]) : C_DGREY);
      if (unlocked) {
        drawBrawlerSprite(50, 60 - bob, 22, sel, 90, false);
        fRect(62, 38, 30, 20, C_YEL); fRect(62, 52, 30, 6, C_BTND);
        drawNum(68, 41, a.level, C_BLK);
      } else {
        shCircRGB(50, 60 - bob, 22, 0x444444, 0x222222, C_GRE);
        drawText(40, 56, "?", C_GRE);
        drawText(22, 102, "A", C_GRE);
        drawNum(34, 102, d.unlockTroph, C_YEL);
        drawText(70, 102, "TROPHEES", C_GRE);
      }
      shadow(50, 88, 16, 4);
      drawText(90, 30, d.name, unlocked ? C_YEL : C_GRE);
      drawText(90, 46, RAR_NAME[d.rar], unlocked ? KDColor::RGB24(RAR_COL[d.rar]) : C_GRE);
      if (unlocked) {
        drawText(90, 62, "DEG", C_WHT);
        drawNum(130, 62, (int)(d.baseDmg * mul * ((a.starPower && d.star == STAR_DMG) ? 1.1f : 1)), C_ORG);
        drawText(180, 62, "PORTEE", C_WHT);
        drawNum(258, 62, d.range, C_CYN);
        drawTextC(80, BRAWLER_BIOS[sel], C_GRE);
      }
      fRect(230, 28, 10, 6, C_GOLD); fRect(233, 35, 8, 3, C_GOLD); fRect(236, 39, 3, 6, C_GOLD);
      drawNum(250, 30, a.trophies, C_WHT);
      oCirc(236, 56, 4, C_YEL, C_BTND); drawNum(250, 52, acc.coins, C_WHT);

      if (unlocked) {
        // super du brawler
        rRect(10, 104, 300, 28, 5, KDColor::RGB24(0x2B3A55));
        drawText(16, 108, "SUPER", C_YEL);
        drawText(16, 120, SUPER_NAMES[sel], C_WHT);
        // gadget / pouvoir star / hypercharge : 3 lignes pleine largeur
        rRect(10, 136, 300, 16, 4, a.gadget ? C_CYN : KDColor::RGB24(0x225566));
        drawText(16, 139, "GADGET", a.gadget ? C_BLK : C_GRE);
        drawText(96, 139, a.gadget ? curGadgetName(sel) : "NON", a.gadget ? C_BLK : C_GRE);
        if (a.gadget) drawNum(284, 139, a.gadChoice + 1, C_BLK);
        rRect(10, 155, 300, 16, 4, a.starPower ? C_PUR : KDColor::RGB24(0x3A1F55));
        drawText(16, 158, "STAR", a.starPower ? C_WHT : C_GRE);
        drawText(70, 158, a.starPower ? curStarName(sel) : "NON", a.starPower ? C_WHT : C_GRE);
        if (a.starPower) drawNum(284, 158, a.starChoice + 1, C_WHT);
        rRect(10, 174, 300, 16, 4, a.hyper ? C_MAG : KDColor::RGB24(0x55114A));
        drawText(16, 177, "HYPER", a.hyper ? C_WHT : C_GRE);
        drawText(80, 177, a.hyper ? "OUI" : (a.level >= 11 ? "DISPO" : "NIV 11"), a.hyper ? C_WHT : C_GRE);

        // boutons d'achat
        cost = upgradeCost(a.level);
        if (a.level < 11) {
          rRect(6, 196, 74, 32, 5, (acc.coins >= cost) ? C_YEL : KDColor::RGB24(0x5A4A9A));
          drawText(12, 199, "+ NIV", C_BLK); drawNum(12, 215, cost, C_BLK);
        } else {
          rRect(6, 196, 74, 32, 5, C_GREEN);
          drawText(12, 207, "NIV MAX", C_BLK);
        }
        if (!a.gadget) { rRect(84, 196, 74, 32, 5, (acc.coins >= 100) ? C_CYN : KDColor::RGB24(0x225566)); drawText(90, 199, "* GADGET", C_BLK); drawNum(90, 215, 100, C_BLK); }
        else { rRect(84, 196, 74, 32, 5, C_CYN); drawText(90, 207, "* CHANGE", C_BLK); }
        if (a.level >= 3 && !a.starPower) { rRect(162, 196, 74, 32, 5, (acc.coins >= 150) ? C_PUR : KDColor::RGB24(0x3A1F55)); drawText(168, 199, "/ STAR", C_WHT); drawNum(168, 215, 150, C_WHT); }
        else if (a.starPower) { rRect(162, 196, 74, 32, 5, C_PUR); drawText(168, 207, "/ CHANGE", C_WHT); }
        if (a.level >= 11 && !a.hyper) { rRect(240, 196, 74, 32, 5, (acc.coins >= 300) ? C_MAG : KDColor::RGB24(0x55114A)); drawText(246, 199, "- HYP", C_WHT); drawNum(246, 215, 300, C_WHT); }
      } else {
        drawTextC(120, "BRAWLER NON DEBLOQUE", C_GRE);
        drawTextC(140, "GAGNE DES TROPHEES POUR", C_WHT);
        drawTextC(156, "LE DEBLOQUER!", C_WHT);
      }
      blit();
      pollKeys();
      animT += 80;
      if (unlocked) {
        // achats par front montant : 1 appui = 1 achat, plus de double comptage
        static bool pPlus = false, pMul = false, pDiv = false, pMinus = false;
        bool doPlus = kPlus && !pPlus, doMul = kMul && !pMul,
             doDiv = kDiv && !pDiv, doMinus = kMinus && !pMinus;
        pPlus = kPlus; pMul = kMul; pDiv = kDiv; pMinus = kMinus;
        if (a.level < 11 && acc.coins >= cost && doPlus)
          { acc.coins -= cost; a.level++; saveAcc(); }
        if (!a.gadget && acc.coins >= 100 && doMul)
          { acc.coins -= 100; a.gadget = true; a.gadChoice = 0; saveAcc(); }
        else if (a.gadget && doMul)
          { a.gadChoice ^= 1; saveAcc(); }
        if (a.level >= 3 && !a.starPower && acc.coins >= 150 && doDiv)
          { acc.coins -= 150; a.starPower = true; a.starChoice = 0; saveAcc(); }
        else if (a.starPower && doDiv)
          { a.starChoice ^= 1; saveAcc(); }
        if (a.level >= 11 && !a.hyper && acc.coins >= 300 && doMinus)
          { acc.coins -= 300; a.hyper = true; saveAcc(); }
      }
      if (keyOK || keyBack) { prevOK = keyOK; prevBack = keyBack; break; }
      prevOK = keyOK; prevBack = keyBack;
      Ion::Timing::msleep(80);
    }
  }
}

// ------------------------- Resultat + star drop -----------------------
static void resultScreen(int result, int myBrawler) {
  int coins = 0, troph = 0;
  if (result == 1) { coins = 50 + rnd(50); troph = 8; }
  else if (result == 0) { coins = 20 + rnd(20); troph = -2; }
  else { coins = 8 + rnd(12); troph = 0; }
  int starDrop = (result == 1 && rnd(100) < 60) ? 20 + rnd(60) : 0;
  acc.coins += coins; // le star drop ne s'ajoute qu'a l'ouverture du coffre
  acc.b[myBrawler].trophies += troph;
  if (acc.b[myBrawler].trophies < 0) acc.b[myBrawler].trophies = 0;
  int newUnlock = -1;
  refreshUnlocks(&newUnlock);
  updateQuests(result);
  saveAcc(); // persistance des trophees / pieces apres chaque match

  const char *msg = (result == 1) ? "VICTOIRE!" : (result == 0) ? "EGALITE" : "DEFAITE";
  KDColor col = (result == 1) ? C_YEL : (result == 0) ? C_WHT : C_RED;
  bool opened = (starDrop <= 0); // pas de coffre si pas de star drop
  int dropUnlock = -1; // star drop rare : peut debloquer un brawleur
  waitRelease();
  while (true) {
    bgPanel();
    rRect(SW / 2 - 100, 20, 200, 42, 10, col);
    drawTextC(34, msg, C_BLK);
    oCirc(SW / 2 - 30, 84, 8, C_YEL, C_BTND);
    drawText(SW / 2 - 44, 80, "+", C_WHT);
    drawNum(SW / 2 - 16, 80, coins, C_WHT);
    fRect(SW / 2 + 20, 76, 10, 5, C_GOLD); fRect(SW / 2 + 22, 81, 6, 2, C_GOLD); fRect(SW / 2 + 24, 83, 2, 5, C_GOLD);
    drawNum(SW / 2 + 36, 80, troph, troph >= 0 ? C_YEL : C_RED);
    if (starDrop > 0) {
      drawTextC(104, "STAR DROP!", C_MAG);
      if (!opened) {
        // coffre ferme : attend un appui OK
        rRect(SW / 2 - 16, 114, 32, 24, 4, C_PUR);
        fRect(SW / 2 - 16, 123, 32, 3, C_MAG);
        oCirc(SW / 2, 126, 3, C_YEL, C_BLK);
        drawTextC(146, "APPUIE OK POUR OUVRIR", C_WHT);
      } else {
        // coffre ouvert : pieces bonus tombent (+ brawleur debloque si chanceux)
        fRect(SW / 2 - 18, 108, 36, 4, C_WHT);
        rRect(SW / 2 - 18, 112, 36, 26, 4, C_MAG);
        drawText(SW / 2 - 16, 120, "+", C_WHT);
        drawNum(SW / 2 - 8, 120, starDrop, C_CYN);
        if (dropUnlock >= 0) {
          drawTextC(146, "BRAWLER DEBLOQUE!", C_GOLD);
          drawTextC(158, BRAWLERS[dropUnlock].name, C_YEL);
        } else drawTextC(146, "PIECES BONUS!", C_CYN);
      }
    }
    if (newUnlock >= 0) {
      const BrawlerDef &d = BRAWLERS[newUnlock];
      int ubY = (starDrop > 0) ? ((dropUnlock >= 0 && opened) ? 166 : 158) : 148;
      rRect(SW / 2 - 110, ubY, 220, 24, 8, KDColor::RGB24(RAR_COL[d.rar]));
      drawTextC(ubY + 3, "NOUVEAU BRAWLER!", C_BLK);
      drawTextC(ubY + 14, d.name, C_WHT);
    }
    rRect(SW / 2 - 70, 190, 140, 24, 8, C_YEL);
    drawTextC(196, opened ? "OK CONTINUER" : "OK OUVRIR", C_BLK);
    blit();
    pollKeys();
    if (keyOK && !prevOK) {
      prevOK = true;
      if (opened) return;
      acc.coins += starDrop; // recompense ajoutee seulement a l'ouverture
      // tres faible chance (4%) que le star drop debloque un brawleur
      if (rnd(100) < 4) {
        int locked[55], nl = 0;
        for (int i = 0; i < 55; i++) if (!isUnlocked(i)) locked[nl++] = i;
        if (nl > 0) { dropUnlock = locked[rnd(nl)]; acc.b[dropUnlock].level = 1; }
      }
      saveAcc();
      opened = true;
    }
    prevOK = keyOK;
    Ion::Timing::msleep(60);
  }
}

static void playMatch(int myBrawler, int selMode) {
  setupMatch(selMode, myBrawler);
  prevOK = prevShift = prevMinus = prevBack = prevBacksp = false; keyBack = false; matchResult = -2;
  while (matchResult == -2) {
    int t0 = (int)Ion::Timing::millis();
    updateMatch(1.0f / 30.0f);
    renderMatch();
    int frameMs = (int)Ion::Timing::millis() - t0;
    Ion::Timing::msleep(frameMs < 33 ? 33 - frameMs : 1);
  }
  resultScreen(matchResult, myBrawler);
}

// ------------------------- Mode Entrainement ---------------------------
// Terrain a cibles : tire sur les cibles qui bougent, mesure ta precision.
static void trainingMode(int myBrawler) {
  struct Target { float x, y, vx, hp, maxHP; bool alive; };
  Target tg[4];
  for (int i = 0; i < 4; i++) {
    tg[i].x = 180 + i * 40; tg[i].y = 60 + (i % 2) * 100;
    tg[i].vx = ((i % 2) ? 1 : -1) * (40 + i * 10);
    tg[i].maxHP = 4000 + i * 1000; tg[i].hp = tg[i].maxHP;
    tg[i].alive = true;
  }
  const BrawlerDef &d = BRAWLERS[myBrawler];
  float px_ = 60, py_ = SH / 2;
  int shots = 0, hits = 0, dmgTotal = 0;
  float ammo = 3, aim = 0;
  Ent dummy; memset(&dummy, 0, sizeof(dummy));
  // projectiles simplifies du mode entrainement
  struct TProj { bool active; float x, y, vx, vy, travelled, range, dmg; };
  TProj tp[16];
  for (int i = 0; i < 16; i++) tp[i].active = false;
  prevOK = prevBack = false; keyBack = false;
  int trainT = 0;
  while (true) {
    pollKeys();
    if (keyBack && !prevBack) { prevBack = true; return; }
    float dt = 1.0f / 30.0f;
    trainT += 33; animT += 33;
    // deplacement
    float spd = 130.0f;
    if (kRight) px_ = fminf(SW - 30, px_ + spd * dt);
    if (kLeft) px_ = fmaxf(20, px_ - spd * dt);
    if (kUp) aim = 0;
    if (kDown) aim = 3.14159f;
    // visee auto vers la cible la plus proche
    float best = 1e9, ax = aim, ay = 0;
    for (int i = 0; i < 4; i++)
      if (tg[i].alive) {
        float dx = tg[i].x - px_, dy = tg[i].y - py_, dd = dx * dx + dy * dy;
        if (dd < best) { best = dd; ax = atan2f(dy, dx); }
      }
    aim = ax; ay = ax;
    if (keyOK && !prevOK && ammo >= 1) {
      shots++;
      ammo -= 1;
      for (int i = 0; i < 16; i++)
        if (!tp[i].active) {
          tp[i].active = true; tp[i].x = px_ + 10; tp[i].y = py_;
          tp[i].vx = cosf(aim) * d.projSpeed; tp[i].vy = sinf(aim) * d.projSpeed;
          tp[i].travelled = 0; tp[i].range = d.range; tp[i].dmg = d.baseDmg;
          break;
        }
    }
    prevOK = keyOK;
    ammo = fminf(3, ammo + dt * 1000.0f / d.reloadMs);
    // deplacement des cibles + projectiles
    for (int i = 0; i < 4; i++)
      if (tg[i].alive) {
        tg[i].x += tg[i].vx * dt;
        if (tg[i].x < 120 || tg[i].x > SW - 20) tg[i].vx = -tg[i].vx;
        if (tg[i].hp <= 0) { tg[i].alive = false; dmgTotal += tg[i].maxHP; }
      }
    for (int i = 0; i < 16; i++)
      if (tp[i].active) {
        tp[i].x += tp[i].vx * dt; tp[i].y += tp[i].vy * dt;
        tp[i].travelled += sqrtf(tp[i].vx * tp[i].vx + tp[i].vy * tp[i].vy) * dt;
        if (tp[i].travelled > tp[i].range) { tp[i].active = false; continue; }
        for (int j = 0; j < 4; j++)
          if (tg[j].alive) {
            float dx = tg[j].x - tp[i].x, dy = tg[j].y - tp[i].y;
            if (dx * dx + dy * dy < 14 * 14) {
              tg[j].hp -= tp[i].dmg; hits++; dmgTotal += (int)tp[i].dmg;
              addPop(tp[i].x, tp[i].y - 12, (int)tp[i].dmg);
              tp[i].active = false; break;
            }
          }
      }
    // toutes les cibles respawnent quand tout est detruit
    bool allDead = true;
    for (int i = 0; i < 4; i++) if (tg[i].alive) allDead = false;
    if (allDead)
      for (int i = 0; i < 4; i++) { tg[i].alive = true; tg[i].hp = tg[i].maxHP; }
    // ---- rendu ----
    fRect(0, 0, SW, SH, KDColor::RGB24(0x4E8A5A));
    for (int ty = 0; ty < SH; ty += TILE)
      for (int tx = 0; tx < SW; tx += TILE)
        if (((tx + ty) / TILE) & 1) fRect(tx, ty, TILE, TILE, KDColor::RGB24(0x447A4E));
    fRect(0, 0, 100, SH, KDColor::RGB24(0x3A3A4A));
    fRect(100, 0, 3, SH, C_BLK);
    for (int i = 0; i < 4; i++)
      if (tg[i].alive) {
        int bx = (int)tg[i].x, by = (int)tg[i].y;
        oCirc(bx, by, 12, C_ORG, C_BLK);
        fCirc(bx, by, 7, C_WHT);
        fCirc(bx, by, 3, C_RED);
        // barre de vie de la cible
        fRect(bx - 12, by - 20, 25, 5, C_BLK);
        fRect(bx - 11, by - 19, (int)(23 * tg[i].hp / tg[i].maxHP), 3, C_YEL);
      }
    for (int i = 0; i < 16; i++)
      if (tp[i].active) oCirc((int)tp[i].x, (int)tp[i].y, 3, C_YEL, C_BLK);
    // joueur
    drawBrawlerSprite((int)px_, (int)py_, 9, myBrawler, (int)(aim * 57.2957795f), false, trainT);
    // munitions
    for (int a = 0; a < 3; a++) {
      int filled = (int)(fmaxf(0, fminf(1, ammo - a)) * 4);
      fRect((int)px_ - 8 + a * 6, (int)py_ + 12, 5, 2, C_BLK);
      fRect((int)px_ - 8 + a * 6, (int)py_ + 12, filled, 2, C_ORG);
    }
    for (int i = 0; i < MAX_POP; i++)
      if (pops[i].active) {
        pops[i].life -= 33; pops[i].y -= 0.7f;
        if (pops[i].life <= 0) pops[i].active = false;
        else drawNum((int)pops[i].x, (int)pops[i].y, pops[i].val, C_WHT);
      }
    // panneau de stats
    drawText(8, 6, "ENTRAINEMENT", C_YEL);
    drawText(8, 24, "TIRS:", C_WHT); drawNum(76, 24, shots, C_WHT);
    drawText(8, 40, "TOUCHES:", C_WHT); drawNum(112, 40, hits, C_CYN);
    drawText(8, 56, "PRECISION:", C_WHT);
    if (shots > 0) drawNum(132, 56, hits * 100 / shots, C_GREEN);
    else drawText(132, 56, "0", C_GREEN);
    drawText(8, 72, "DEGATS:", C_WHT); drawNum(96, 72, dmgTotal, C_ORG);
    drawTextC(SH - 14, "OK TIR  BACK QUITTER", C_GRE);
    blit();
    Ion::Timing::msleep(20);
  }
}

// ------------------------- Point d'entree ------------------------------
void ion_main(int argc, char *argv[]) {
  (void)argc; (void)argv;
  initAcc(); // charge le compte ET les quetes sauvegardees (initQuests si vierge)
  int myBrawler = 0;
  const char *mainItems[7] = {"JOUER", "ENTRAINEMENT", "BRAWLERS", "QUETES DU JOUR", "ROUTE DES TROPHEES", "PARAMETRES", "AIDE"};
  while (true) {
    int choice = menuPick("BRAWL MICROSTARS", mainItems, 7, 0);
    if (choice < 0) return;
    waitRelease();
    if (choice == 0) {
      const char *modes[MODE_COUNT];
      for (int i = 0; i < MODE_COUNT; i++) modes[i] = MODE_NAMES[i];
      gMenuDesc = MODE_DESC; // affiche la description du mode selectionne
      int selMode = menuPick("MODE DE JEU", modes, MODE_COUNT, 0);
      gMenuDesc = nullptr;
      if (selMode < 0) { waitRelease(); continue; }
      waitRelease();
      // selection du brawler (V5) : on choisit qui on joue
      int picked = brawlerPicker(myBrawler);
      if (picked < 0) { waitRelease(); continue; }
      myBrawler = picked;
      waitRelease();
      playMatch(myBrawler, selMode);
    } else if (choice == 1) {
      trainingMode(myBrawler);
    } else if (choice == 2) {
      brawlerMenu(myBrawler);
    } else if (choice == 3) {
      questScreen();
    } else if (choice == 4) {
      trophyRoadScreen();
    } else if (choice == 5) {
      settingsScreen();
    } else {
      bgPanel();
      drawTextC(8, "AIDE", C_YEL);
      fRect(SW / 2 - 60, 22, 120, 3, C_BTND);
      const char *lines[8] = {
        "FLECHES: BOUGER", "OK/VAR: ATTAQUE",
        "RETOUR: ULTI", "SHIFT: GADGET",
        "EFFACER: HYPERCHARGE", "HOME: QUITTER",
        "GAGNE DES TROPHEES POUR", "DEBLOQUER DES BRAWLERS"
      };
      for (int i = 0; i < 8; i++) drawTextC(38 + i * 17, lines[i], C_WHT);
      rRect(SW / 2 - 70, 184, 140, 24, 8, C_YEL);
      drawTextC(190, "OK RETOUR", C_BLK);
      blit();
      prevOK = true;
      while (prevOK) { pollKeys(); prevOK = keyOK; Ion::Timing::msleep(60); }
      prevOK = false;
    }
  }
}

// --- Entry point reel : sans main, --gc-sections supprime tout le jeu ---
// np_app_begin/end (depuis epsilon_app.h) : maintient Home bloque pendant
// le jeu et rend la memoire proprement a Epsilon en sortant.
extern "C" int main(int argc, char *argv[]) {
  np_app_begin();
  ion_main(argc, argv);
  return np_app_end();
}
