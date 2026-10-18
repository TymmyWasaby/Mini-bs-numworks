// ===================================================================
//  BRAWL MICROSTARS v4 — Shooter 2D type Brawl Stars pour NumWorks
//  App externe Epsilon (.nwa via nwlink / EADK)
//
//  CONTROLES :
//    Fleches : bouger | OK : attaque | Shift : Super (mega attaque)
//    X : Gadget | - : Hypercharge | Back : quitter
//    + * / : achats dans le menu BRAWLERS
//
//  V4 : SPRITES UNIQUE PAR BRAWLER (coiffure/chapeau/accessoire),
//       THEMES DE CARTE UNIQUE PAR MODE, rendu enrichi style BS.
//  8 MODES, 50 BRAWLERS, Route des Trophees, star drops, IA adaptee.
// ===================================================================

#include <ion.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SW 320
#define SH 240
#define TILE 14
#define MAPW 32
#define MAPH 24

static KDColor frameBuf[SW * SH];
static uint16_t seed = 12345;
static int rnd(int n) { seed = (uint16_t)(seed * 25173 + 13849); return (seed >> 4) % (n > 0 ? n : 1); }

static inline void px(int x, int y, KDColor c) {
  if (x >= 0 && x < SW && y >= 0 && y < SH) frameBuf[y * SW + x] = c;
}
static void fRect(int x, int y, int w, int h, KDColor c) {
  if (w < 0) { x += w; w = -w; } if (h < 0) { y += h; h = -h; }
  for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) px(i, j, c);
}
static void rRect(int x, int y, int w, int h, int r, KDColor c) {
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
    int dx = (i < r) ? (r - i) : ((i >= w - r) ? (i - (w - r - 1)) : 0);
    int dy = (j < r) ? (r - j) : ((j >= h - r) ? (j - (h - r - 1)) : 0);
    if (dx * dx + dy * dy <= r * r) px(x + i, y + j, c);
  }
}
static void fCirc(int cx, int cy, int r, KDColor c) {
  for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++)
    if (i * i + j * j <= r * r) px(cx + i, cy + j, c);
}
static void oCirc(int cx, int cy, int r, KDColor fill, KDColor outline) {
  for (int j = -r - 1; j <= r + 1; j++) for (int i = -r - 1; i <= r + 1; i++) {
    int d2 = i * i + j * j;
    if (d2 <= r * r) px(cx + i, cy + j, fill);
    else if (d2 <= (r + 1) * (r + 1)) px(cx + i, cy + j, outline);
  }
}
static void shCircRGB(int cx, int cy, int r, uint32_t topRGB, uint32_t botRGB, KDColor outline) {
  for (int j = -r - 1; j <= r + 1; j++) for (int i = -r - 1; i <= r + 1; i++) {
    int d2 = i * i + j * j;
    if (d2 <= r * r) {
      float t = (float)(j + r) / (2.0f * r);
      KDColor top = KDColor::RGB24(topRGB), bot = KDColor::RGB24(botRGB);
      px(cx + i, cy + j, KDColor::RGB(
        (uint8_t)(top.red() + (bot.red() - top.red()) * t),
        (uint8_t)(top.green() + (bot.green() - top.green()) * t),
        (uint8_t)(top.blue() + (bot.blue() - top.blue()) * t)));
    } else if (d2 <= (r + 1) * (r + 1)) px(cx + i, cy + j, outline);
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
  for (int j = -ry; j <= ry; j++) for (int i = -rx; i <= rx; i++)
    if ((i * i * 100) / (rx * rx + 1) + (j * j * 100) / (ry * ry + 1) <= 100)
      px(cx + i, cy + j, KDColor::RGB24(0x0A1608));
}
static void blit() { Ion::Display::pushRect(KDRect(0, 0, SW, SH), frameBuf); }

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
enum SuperType { SUP_BURST, SUP_DASH, SUP_HEAL, SUP_BARRAGE, SUP_MEGASHOT };
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

static const BrawlerDef BRAWLERS[50] = {
  {"Shelly",   5600, 1280, 170, 1500, 340, 3, 24, ATK_SPREAD, SUP_BURST,    GAD_SPEED, STAR_DMG,    0,     0, 0x8A4BE0, 0x5C2BA0, 0},
  {"Nita",     5400, 1320, 160, 1400, 330, 1,  0, ATK_SHOT,   SUP_BURST,    GAD_HEAL,  STAR_HP,    0,     0, 0xC98A3B, 0x8A5C22, 1},
  {"Colt",     4000,  940, 240, 1600, 420, 6,  8, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   0,     0, 0x3B9AC9, 0x22628A, 1},
  {"Bull",     6600, 1750, 100, 1900, 300, 5, 30, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    1,    60, 0xC9973B, 0x8A6622, 0},
  {"Brock",    3800, 1620, 280, 2000, 360, 1,  0, ATK_LOB,    SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   1,   120, 0xC93B3B, 0x8A2222, 2},
  {"El Primo", 6800, 1400,  90, 1300, 330, 4, 20, ATK_SPREAD, SUP_DASH,     GAD_HEAL,  STAR_HP,    1,   180, 0x3BA05C, 0x22663A, 0},
  {"Barley",   3600, 1160, 240, 1700, 260, 1,  0, ATK_LOB,    SUP_BARRAGE,  GAD_AMMO,  STAR_REGEN, 1,   240, 0x7BC93B, 0x4F8A22, 3},
  {"Poco",     5600, 1100, 200, 1400, 300, 3, 16, ATK_SPREAD, SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 1,   300, 0xB03BE0, 0x7822A0, 1},
  {"Rosa",     6400, 1250, 120, 1500, 340, 3, 12, ATK_SPREAD, SUP_HEAL,     GAD_SPEED, STAR_HP,    2,   400, 0x7AC96B, 0x4F8A46, 0},
  {"Jessie",   4600, 1180, 210, 1700, 300, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_REGEN, 2,   500, 0xA03BA0, 0x701F70, 2},
  {"Dynamike", 4000, 1330, 230, 1800, 280, 2, 12, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_DMG,   2,   620, 0xE0B03B, 0xA07822, 3},
  {"Tick",     3000, 1120, 260, 1500, 260, 1,  0, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_REGEN, 2,   750, 0x8AC93B, 0x5C8A22, 3},
  {"8-Bit",    5800, 1350, 260, 2000, 330, 1,  0, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   2,   880, 0xC9A03B, 0x8A6E22, 1},
  {"Rico",     4200, 1010, 260, 1500, 400, 5,  6, ATK_SHOT,   SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   2,  1000, 0x3BC9B0, 0x228A78, 1},
  {"Darryl",   6300, 1460, 130, 1400, 380, 2, 14, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    2,  1150, 0xC96B3B, 0x8A4822, 0},
  {"Penny",    4600, 1290, 230, 1700, 340, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_HEAL,  STAR_REGEN, 2,  1300, 0xE07B3B, 0xA05222, 0},
  {"Carl",     5500, 1250, 190, 1200, 320, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_HP,    3,  1500, 0x7B9BC9, 0x50688A, 3},
  {"Jacky",    6000, 1300, 110, 1500, 320, 4, 22, ATK_SPREAD, SUP_BURST,    GAD_BLAST, STAR_HP,    3,  1700, 0xE0B03B, 0xA07822, 0},
  {"Bo",       5000, 1200, 250, 1800, 310, 3,  9, ATK_LOB,    SUP_BARRAGE,  GAD_SPEED, STAR_REGEN, 3,  1900, 0x3B7AC9, 0x224F8A, 2},
  {"Emz",      4800, 1180, 200, 1600, 280, 2, 18, ATK_SPREAD, SUP_BURST,    GAD_HEAL,  STAR_REGEN, 3,  2100, 0xE03B8A, 0xA02258, 1},
  {"Stu",      3800, 1080, 220, 1300, 420, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_DMG,   3,  2350, 0xC93BA0, 0x8A2270, 2},
  {"Pam",      6200, 1150, 220, 1800, 300, 4, 10, ATK_SPREAD, SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 3,  2600, 0xB09B3B, 0x7A6A22, 2},
  {"Frank",    7400, 1800, 140, 2400, 260, 1,  0, ATK_SHOT,   SUP_BURST,    GAD_AMMO,  STAR_HP,    3,  2900, 0x5C7A9B, 0x3B5068, 3},
  {"Bibi",     5900, 1350, 120, 1300, 360, 3, 15, ATK_SPREAD, SUP_DASH,     GAD_SPEED, STAR_HP,    3,  3200, 0xE05C3B, 0xA03E22, 0},
  {"Bea",      3600,  960, 270, 1500, 380, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   3,  3500, 0xE0C93B, 0xA08A22, 2},
  {"Nani",     3400, 1420, 260, 1900, 340, 1,  0, ATK_SHOT,   SUP_MEGASHOT, GAD_SPEED, STAR_DMG,   4,  3900, 0x9B7BE0, 0x6850A0, 2},
  {"Edgar",    4200, 1050,  80, 1100, 380, 2, 10, ATK_SPREAD, SUP_DASH,     GAD_HEAL,  STAR_REGEN, 4,  4300, 0x3B3B50, 0x22223A, 0},
  {"Griff",    4900, 1080, 230, 1500, 360, 3,  8, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   4,  4700, 0xC9C93B, 0x8A8A22, 1},
  {"Gale",     5300, 1240, 200, 1600, 320, 3, 14, ATK_SPREAD, SUP_BURST,    GAD_SPEED, STAR_HP,    4,  5100, 0x7BC9E0, 0x508A9B, 2},
  {"Colette",  4800, 1300, 210, 1500, 330, 2,  6, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   4,  5500, 0xE05C9B, 0xA03E68, 1},
  {"Belle",    3900, 1150, 260, 1700, 350, 1,  0, ATK_SHOT,   SUP_BARRAGE,  GAD_SPEED, STAR_DMG,   4,  6000, 0x7B3BE0, 0x5022A0, 2},
  {"Buzz",     6100, 1380, 110, 1300, 370, 3, 18, ATK_SPREAD, SUP_DASH,     GAD_BLAST, STAR_HP,    4,  6500, 0x3BE0C9, 0x229B8A, 0},
  {"Ash",      6500, 1290, 180, 1700, 300, 4, 12, ATK_SHOT,   SUP_BURST,    GAD_AMMO,  STAR_HP,    4,  7000, 0x9BA05C, 0x686A3A, 3},
  {"Lola",     4400, 1220, 240, 1600, 340, 1,  0, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   4,  7500, 0xE03B5C, 0xA0223E, 1},
  {"Fang",     5700, 1420, 140, 1400, 350, 2, 12, ATK_SPREAD, SUP_DASH,     GAD_SPEED, STAR_HP,    4,  8000, 0xC95C3B, 0x8A3E22, 0},
  {"Maisie",   4700, 1260, 240, 1700, 330, 1,  0, ATK_SHOT,   SUP_BURST,    GAD_BLAST, STAR_DMG,   5,  8600, 0x3BE0A0, 0x229B68, 2},
  {"Squeak",   3500, 1200, 250, 1800, 280, 1,  0, ATK_LOB,    SUP_BARRAGE,  GAD_BLAST, STAR_REGEN, 5,  9200, 0xC93BE0, 0x8A22A0, 3},
  {"Hank",     6900, 1500, 220, 2000, 290, 3, 10, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_HP,    5,  9800, 0x3B6BE0, 0x2247A0, 2},
  {"Pearl",    5200, 1230, 210, 1500, 320, 2,  8, ATK_SPREAD, SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 5, 10500, 0xE09B3B, 0xA06822, 1},
  {"Spike",    3200, 1120, 250, 1900, 300, 3, 20, ATK_LOB,    SUP_BURST,    GAD_SPEED, STAR_DMG,   5, 11200, 0x7BE05C, 0x4FA03E, 3},
  {"Crow",     3200, 1080, 260, 1500, 400, 3,  6, ATK_SHOT,   SUP_BARRAGE,  GAD_SPEED, STAR_DMG,   5, 12000, 0x5C3BE0, 0x3E22A0, 2},
  {"Leon",     4200, 1160, 220, 1300, 390, 4,  7, ATK_SPREAD, SUP_BURST,    GAD_SPEED, STAR_DMG,   5, 12800, 0x3BE07B, 0x22A050, 1},
  {"Sandy",    4600, 1180, 240, 1600, 320, 3,  9, ATK_LOB,    SUP_BARRAGE,  GAD_AMMO,  STAR_REGEN, 5, 13700, 0x7B7BE0, 0x5050A0, 2},
  {"Amber",    4800, 1090, 200, 1200, 340, 2,  4, ATK_SHOT,   SUP_BARRAGE,  GAD_AMMO,  STAR_DMG,   5, 14600, 0xE0C95C, 0xA08A3E, 1},
  {"Gene",     4500, 1230, 250, 1700, 310, 1,  0, ATK_SHOT,   SUP_BURST,    GAD_HEAL,  STAR_REGEN, 5, 15500, 0x9B3BE0, 0x6822A0, 2},
  {"Mortis",   5000, 1260, 150, 1300, 380, 1,  0, ATK_SHOT,   SUP_DASH,     GAD_SPEED, STAR_HP,    5, 16500, 0x3B3B5C, 0x22223E, 3},
  {"Byron",    3900, 1050, 270, 1800, 340, 1,  0, ATK_SHOT,   SUP_HEAL,     GAD_HEAL,  STAR_REGEN, 5, 17500, 0xC9C9C9, 0x8A8A8A, 2},
  {"Meg",      4100, 1150, 230, 1500, 330, 2,  9, ATK_SHOT,   SUP_MEGASHOT, GAD_AMMO,  STAR_DMG,   5, 18600, 0xE03BC9, 0xA0228A, 1},
  {"Chester",  4600, 1210, 210, 1400, 350, 3, 11, ATK_SPREAD, SUP_BARRAGE,  GAD_BLAST, STAR_DMG,   5, 19800, 0x5CC9E0, 0x3E8A9B, 0},
  {"Surge",    4500, 1250, 200, 1400, 360, 3, 13, ATK_SPREAD, SUP_BURST,    GAD_SPEED, STAR_DMG,   5, 21000, 0x3BE0E0, 0x229B9B, 2},
};

// ------------- Traits visuels uniques par brawler --------------------
// Derives deterministes de l'index : chaque brawler a sa coiffure,
// son chapeau/accessoire et sa couleur d'accent. 50 looks differents.
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
static void drawBrawlerSprite(int cx, int cy, int r, int bIdx, int faceAng, bool flash) {
  const BrawlerDef &d = BRAWLERS[bIdx];
  Look L; brawlerLook(bIdx, L);
  int rr = r + (L.size == 2 ? 2 : (L.size == 0 ? -1 : 0));
  KDColor acc = KDColor::RGB24(L.accent);
  // corps degrade
  shCircRGB(cx, cy, rr, flash ? 0xFFFFFF : d.colTop, flash ? 0x9A9A9A : d.colBot, C_BLK);
  // arme selon type
  int gx = (int)(cosf(faceAng * 0.01745f) * (rr + 3)), gy = (int)(sinf(faceAng * 0.01745f) * (rr + 3));
  KDColor wc = KDColor::RGB24(0x6A4A2A);
  if (L.weapon == 1) { fRect(cx + gx - 2, cy + gy - 2, 6, 2, wc); fRect(cx + gx + 3, cy + gy - 1, 2, 3, C_GRE); }
  else if (L.weapon == 2) { fRect(cx + gx - 1, cy + gy - 3, 2, 7, wc); fCirc(cx + gx, cy + gy - 4, 2, C_GRE); }
  else if (L.weapon == 3) { fRect(cx + gx - 1, cy + gy - 3, 2, 7, wc); fRect(cx + gx - 3, cy + gy - 4, 6, 2, C_GRE); }
  else if (L.weapon == 4) { fRect(cx + gx - 1, cy + gy - 3, 2, 8, wc); fCirc(cx + gx, cy + gy + 4, 2, acc); }
  // yeux
  int ex = (int)(cosf(faceAng * 0.01745f) * 2), ey = (int)(sinf(faceAng * 0.01745f) * 2);
  if (L.eyes == 0) { fRect(cx + ex - 3, cy + ey - 2, 2, 3, C_WHT); fRect(cx + ex + 1, cy + ey - 2, 2, 3, C_WHT); }
  else if (L.eyes == 1) { fRect(cx + ex - 3, cy + ey - 1, 2, 1, C_WHT); fRect(cx + ex + 1, cy + ey - 1, 2, 1, C_WHT); }
  else { fRect(cx + ex - 4, cy + ey - 3, 3, 4, C_WHT); fRect(cx + ex + 1, cy + ey - 3, 3, 4, C_WHT); }
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
}

// ------------------------- Compte / progression ----------------------
struct BrawlerAccount { int level, trophies; bool gadget, starPower, hyper; };
struct Account { BrawlerAccount b[50]; int coins; } acc;
static int unlockedBrawlers = 0;

static int upgradeCost(int lvl) { return 20 + lvl * 30; }
static float lvlMul(int lvl) { return 1.0f + (lvl - 1) * 0.05f; }

static int totalTrophies() {
  int t = 0;
  for (int i = 0; i < 50; i++) t += acc.b[i].trophies;
  return t;
}
static bool isUnlocked(int i) { return acc.b[i].level > 0; }

static void refreshUnlocks(int *newlyUnlocked) {
  *newlyUnlocked = -1;
  int tt = totalTrophies();
  for (int i = 0; i < 50; i++)
    if (acc.b[i].level == 0 && tt >= BRAWLERS[i].unlockTroph) {
      acc.b[i].level = 1; acc.b[i].trophies = 0;
      acc.b[i].gadget = false; acc.b[i].starPower = false; acc.b[i].hyper = false;
      *newlyUnlocked = i;
    }
  unlockedBrawlers = 0;
  for (int i = 0; i < 50; i++) if (isUnlocked(i)) unlockedBrawlers++;
}

static void initAcc() {
  memset(&acc, 0, sizeof(acc));
  acc.coins = 100;
  int dummy;
  refreshUnlocks(&dummy);
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
#define MAX_ENT 8
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
  float aiSkill;
  int walkT, lastHitT;
  bool drawDone;
};
static Ent ents[MAX_ENT]; static int entCount = 0;
static int humanIdx = -1;

struct Proj {
  bool active, lob, fromSuper;
  float x, y, vx, vy, dmg, travelled, range;
  int team, owner, radius, shape, spin;
};
static Proj projs[MAX_PROJ];

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
static void moveEnt(Ent &e, float dx, float dy) {
  if (solidAt(e.x + dx, e.y)) dx = 0;
  if (solidAt(e.x, e.y + dy)) dy = 0;
  e.x += dx; e.y += dy;
  e.x = fmaxf(8, fminf(MAPW * TILE - 8, e.x));
  e.y = fmaxf(8, fminf(MAPH * TILE - 8, e.y));
  if (dx || dy) { e.faceAng = (int)(atan2f(dy, dx) * 57.2957795f) & 255; e.walkT += (int)(fabsf(dx) + fabsf(dy)); }
}

// ------------------------- Generation carte --------------------------
static void genMap() {
  for (int x = 0; x < MAPW; x++)
    for (int y = 0; y < MAPH; y++) {
      tileMap[x][y] = 0;
      if (x == 0 || y == 0 || x == MAPW - 1 || y == MAPH - 1) tileMap[x][y] = 1;
    }
  for (int t = 0; t < 95; t++) {
    int x = 1 + rnd(MAPW / 2), y = 1 + rnd(MAPH - 2);
    int v = rnd(10) < 4 ? 2 : 1;
    if (x < 5 || abs((int)(y - MAPH / 2)) < 2) continue;
    tileMap[x][y] = v;
    tileMap[MAPW - 1 - x][MAPH - 1 - y] = v;
  }
  if (mode == MODE_BALL || mode == MODE_HEIST || mode == MODE_ZONE)
    for (int x = 1; x < MAPW - 1; x++)
      for (int y = MAPH / 2 - 1; y <= MAPH / 2 + 1; y++) tileMap[x][y] = 0;
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
  e.aiSkill = aiSkill;
  e.walkT = 0; e.lastHitT = 0; e.drawDone = false;
}

static void setupMatch(int selMode, int myBrawler) {
  mode = selMode; entCount = 0; humanIdx = -1;
  for (int i = 0; i < MAX_PROJ; i++) projs[i].active = false;
  for (int i = 0; i < MAX_POP; i++) pops[i].active = false;
  for (int i = 0; i < 10; i++) gems[i].active = false;
  for (int i = 0; i < MAX_SAFE; i++) { safes[i].hp = 0; safes[i].maxHP = 0; }
  for (int i = 0; i < MAX_ZONE; i++) zones[i].prog = 0;
  genMap();
  for (int i = 0; i < 5; i++) { teamGems[i] = 0; teamStars[i] = 0; }
  gemCountdown = -1; matchTimeMs = 0; shakeT = 0; poisonWaveT = 0;
  float skill = aiSkillFromTrophies();

  int pool[50], pn = 0;
  for (int i = 0; i < 50; i++) if (isUnlocked(i)) pool[pn++] = i;
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
    poisonWaveT = 25000;
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
}

// ------------------------- Projectiles ------------------------------
static void fireProj(Ent &e, float ang, bool isSuper, float dmgMul, float spdMul, float rangeMul) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  for (int i = 0; i < MAX_PROJ; i++) {
    if (projs[i].active) continue;
    Proj &p = projs[i];
    p.active = true; p.lob = (d.atk == ATK_LOB); p.fromSuper = isSuper;
    p.x = e.x; p.y = e.y;
    float sp = d.projSpeed * spdMul;
    p.vx = cosf(ang) * sp; p.vy = sinf(ang) * sp;
    p.dmg = d.baseDmg * dmgMul;
    p.range = d.range * rangeMul;
    p.travelled = 0; p.team = e.team; p.owner = (&e - ents);
    p.radius = isSuper ? 4 : 2;
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
  int n = d.projCount, spread = d.spreadDeg;
  for (int i = 0; i < n; i++) {
    float a = ang + ((n > 1) ? (spread * 0.01745f * (i - (n - 1) / 2.0f)) : 0);
    fireProj(e, a, false, mul, 1.0f, 1.0f);
  }
  e.ammo -= 1; e.regenT = 0;
}

static void doSuper(Ent &e, float ang) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  if (e.superCharge < 100) return;
  e.superCharge = 0; e.aimAng = ang;
  float mul = lvlMul(e.accIdx >= 0 ? acc.b[e.accIdx].level : 1);
  if (e.hyperT > 0) mul *= 1.25f;
  shakeT = 200; shakeMag = 3;
  switch (d.sup) {
    case SUP_BURST: {
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].team != e.team) {
          float dx = ents[i].x - e.x, dy = ents[i].y - e.y;
          if (dx * dx + dy * dy < 80 * 80) { ents[i].hp -= d.baseDmg * 1.6f * mul; ents[i].regenT = 0; ents[i].lastHitT = 150; }
        }
      for (int a = 0; a < 16; a++) fireProj(e, a * 0.3927f, true, mul * 0.6f, 1.2f, 0.5f);
      break;
    }
    case SUP_DASH: { e.dashT = 0.35f; e.dashDx = (int)(cosf(ang) * 320); e.dashDy = (int)(sinf(ang) * 320); break; }
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
    case SUP_MEGASHOT: { fireProj(e, ang, true, mul * 2.5f, 1.3f, 1.6f); break; }
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
static bool keyOK, keyBack, prevOK, prevShift, prevX, prevMinus, prevBack;
static bool kUp, kDown, kLeft, kRight, kShift, kX, kMinus;

static void pollKeys() {
  Ion::Keyboard::State st = Ion::Keyboard::getState();
  using K = Ion::Keyboard::Key;
  kUp = st.keyDown(K::Up); kDown = st.keyDown(K::Down);
  kLeft = st.keyDown(K::Left); kRight = st.keyDown(K::Right);
  keyOK = st.keyDown(K::OK) || st.keyDown(K::EXE);
  kShift = st.keyDown(K::Shift); kX = st.keyDown(K::X);
  kMinus = st.keyDown(K::Minus); keyBack = st.keyDown(K::Back);
}

// ------------------------- IA ---------------------------------------
static float nearestEnemyDist(Ent &e, Ent **out) {
  float best = 1e9; *out = nullptr;
  for (int i = 0; i < entCount; i++) {
    Ent &o = ents[i];
    if (!o.alive || o.team == e.team) continue;
    if (inBush(o.x, o.y) && !e.isHuman) continue;
    float dx = o.x - e.x, dy = o.y - e.y, d2 = dx * dx + dy * dy;
    if (d2 < best) { best = d2; *out = &o; }
  }
  return sqrtf(best);
}

static void updateAI(Ent &e, float dt) {
  const BrawlerDef &d = BRAWLERS[e.brawlerIdx];
  float sk = e.aiSkill;
  e.aiThinkT -= (int)(dt * 1000);
  if (e.aiThinkT <= 0) {
    e.aiThinkT = (int)(450 - 250 * sk) + rnd(200);
    float tx = e.x, ty = e.y;
    if (mode == MODE_GEM)
      for (int i = 0; i < 10; i++)
        if (gems[i].active) { tx = gems[i].x; ty = gems[i].y; break; }
    else if (mode == MODE_BALL && !e.hasBall && ballVX == 0 && ballVY == 0) { tx = ballX; ty = ballY; }
    else if (mode == MODE_HEIST) {
      for (int s = 0; s < MAX_SAFE; s++)
        if (safes[s].maxHP > 0 && safes[s].hp > 0 && safes[s].team != e.team) { tx = safes[s].x; ty = safes[s].y; }
    }
    else if (mode == MODE_ZONE) {
      float bestP = 1e9;
      for (int z = 0; z < MAX_ZONE; z++)
        if (zones[z].prog < bestP) { bestP = zones[z].prog; tx = zones[z].x; ty = zones[z].y; }
    }
    Ent *tgt = nullptr;
    float ed = nearestEnemyDist(e, &tgt);
    if (tgt && ed < d.range * (0.9f + 0.3f * sk)) { tx = tgt->x; ty = tgt->y; }
    if (mode == MODE_BALL && e.hasBall) { tx = (e.team == 0) ? MAPW * TILE - 20 : 20; ty = MAPH * TILE / 2; }
    if ((mode == MODE_SOLO || mode == MODE_DUO) && e.hp < e.maxHP * (0.4f - 0.2f * sk) && tgt && ed < 200) {
      tx = e.x - (tgt->x - e.x); ty = e.y - (tgt->y - e.y);
    }
    e.aiWx = tx; e.aiWy = ty;
    if (tgt && ed < d.range * 0.95f && e.ammo >= 1) {
      float err = (1.0f - sk) * 12.0f;
      doAttack(e, atan2f(tgt->y - e.y, tgt->x - e.x) + (rnd(100) / 100.0f - 0.5f) * err * 0.01745f);
    }
    if (e.superCharge >= 100 && tgt && ed < d.range)
      doSuper(e, atan2f(tgt->y - e.y, tgt->x - e.x));
    if (e.gadgetUses > 0 && e.hp < e.maxHP * 0.35f) doGadget(e);
  }
  float dx = e.aiWx - e.x, dy = e.aiWy - e.y;
  float len = sqrtf(dx * dx + dy * dy);
  if (len > 10) {
    float sp = 90 + 30 * sk;
    moveEnt(e, dx / len * sp * dt, dy / len * sp * dt);
  }
}

// ------------------------- Match --------------------------------------
static void damageEnt(Ent &e, float dmg, Ent &src) {
  e.hp -= dmg; e.regenT = 0; e.lastHitT = 150;
  src.superCharge = fminf(100, src.superCharge + (src.hyperT > 0 ? 3.5f : 2.0f));
  addPop(e.x + rnd(8) - 4, e.y - 12, (int)dmg);
}

static void updateMatch(float dt) {
  matchTimeMs += (int)(dt * 1000);
  animT += (int)(dt * 1000);
  if (shakeT > 0) shakeT -= (int)(dt * 1000);
  bool ffa = (mode == MODE_SOLO || mode == MODE_DUO);
  bool noRespawn = ffa || mode == MODE_KO;

  for (int i = 0; i < MAX_POP; i++)
    if (pops[i].active) { pops[i].life -= (int)(dt * 1000); pops[i].y -= dt * 22; if (pops[i].life <= 0) pops[i].active = false; }

  Ent &me = ents[humanIdx];
  pollKeys();
  if (keyBack) { matchResult = 0; return; }
  if (me.alive) {
    float spd = 110.0f;
    if (me.speedBuffT > 0) spd *= 1.3f;
    if (me.hyperT > 0) spd *= 1.15f;
    const BrawlerDef &d = BRAWLERS[me.brawlerIdx];
    if (me.accIdx >= 0 && acc.b[me.accIdx].starPower && d.star == STAR_SPEED) spd *= 1.1f;
    float dx = (kRight ? 1 : 0) - (kLeft ? 1 : 0);
    float dy = (kDown ? 1 : 0) - (kUp ? 1 : 0);
    if (dx || dy) { float l = sqrtf(dx * dx + dy * dy); moveEnt(me, dx / l * spd * dt, dy / l * spd * dt); }
    if (keyOK && !prevOK) {
      Ent *tgt = nullptr; nearestEnemyDist(me, &tgt);
      float ang = tgt ? atan2f(tgt->y - me.y, tgt->x - me.x) : (dy || dx ? atan2f(dy, dx) : me.aimAng);
      doAttack(me, ang);
    }
    if (kShift && !prevShift) {
      Ent *tgt = nullptr; nearestEnemyDist(me, &tgt);
      float ang = tgt ? atan2f(tgt->y - me.y, tgt->x - me.x) : me.aimAng;
      doSuper(me, ang);
    }
    if (kX && !prevX) doGadget(me);
    if (kMinus && !prevMinus) doHypercharge(me);
  }
  prevOK = keyOK; prevShift = kShift; prevX = kX; prevMinus = kMinus;

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
    if (!e.isHuman) updateAI(e, dt);
    if (e.speedBuffT > 0) e.speedBuffT -= dt;
    if (e.hyperT > 0) e.hyperT -= dt;
    if (e.lastHitT > 0) e.lastHitT -= (int)(dt * 1000);
    if (e.dashT > 0) {
      e.dashT -= dt;
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
      e.alive = false; e.respawnT = respawnMs;
      if (e.gems > 0)
        for (int g = 0; g < 10 && e.gems > 0; g++)
          if (!gems[g].active) { gems[g].active = true; gems[g].x = e.x + rnd(20) - 10; gems[g].y = e.y + rnd(20) - 10; e.gems--; }
      if (e.hasBall) { e.hasBall = false; ballX = e.x; ballY = e.y; }
      shakeT = 200; shakeMag = 3;
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
    if (p.travelled > p.range) { p.active = false; continue; }
    if (!p.lob && solidAt(p.x, p.y)) { p.active = false; continue; }
    if (p.lob && p.travelled > p.range * 0.6f && solidAt(p.x, p.y)) { p.active = false; continue; }
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
    for (int j = 0; j < entCount; j++) {
      Ent &t = ents[j];
      if (!t.alive || t.team == p.team) continue;
      float ddx = t.x - p.x, ddy = t.y - p.y;
      if (ddx * ddx + ddy * ddy < (p.radius + 7) * (p.radius + 7)) {
        damageEnt(t, p.dmg, ents[p.owner]);
        p.active = false; break;
      }
    }
  }

  if (mode == MODE_GEM) {
    for (int t = 0; t < 5; t++) teamGems[t] = 0;
    for (int i = 0; i < entCount; i++) if (ents[i].alive) teamGems[ents[i].team] += ents[i].gems;
    for (int g = 0; g < 10; g++) {
      if (!gems[g].active) continue;
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && fabsf(ents[i].x - gems[g].x) < 12 && fabsf(ents[i].y - gems[g].y) < 12) {
          gems[g].active = false; ents[i].gems++; break;
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
      if (gemCountdown <= 0) matchResult = (teamGems[0] >= 10) ? 1 : -1;
    } else gemCountdown = -1;
  }

  if (mode == MODE_BALL) {
    bool carried = false;
    for (int i = 0; i < entCount; i++) if (ents[i].alive && ents[i].hasBall) carried = true;
    if (!carried) {
      for (int i = 0; i < entCount; i++) {
        Ent &e = ents[i];
        if (!e.alive || e.hasBall) continue;
        if (fabsf(e.x - ballX) < 12 && fabsf(e.y - ballY) < 12) { e.hasBall = true; break; }
      }
      if (!carried) {
        ballX += ballVX * dt; ballY += ballVY * dt;
        ballVX *= 0.95f; ballVY *= 0.95f;
        if (solidAt(ballX, ballY)) { ballX -= ballVX * dt * 2; ballY -= ballVY * dt * 2; ballVX = ballVY = 0; }
      }
    } else {
      for (int i = 0; i < entCount; i++)
        if (ents[i].alive && ents[i].hasBall) {
          ballX = ents[i].x; ballY = ents[i].y;
          if ((ents[i].team == 0 && ballX > MAPW * TILE - 26) ||
              (ents[i].team == 1 && ballX < 26)) {
            ents[i].goals++; ents[i].hasBall = false;
            ballX = MAPW * TILE / 2; ballY = MAPH * TILE / 2; ballVX = ballVY = 0;
            shakeT = 400; shakeMag = 5;
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
        if (!ents[i].alive) continue;
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
    if (poisonWaveT <= 0) { poisonWaveT = 4000; soloShrink -= (mode == MODE_KO) ? 90 : 60; }
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
        if (ents[i].alive) { if (ents[i].team == 0) alive0++; else alive1++; }
      if (alive0 == 0) matchResult = -1;
      if (alive1 == 0) matchResult = 1;
    } else {
      int aliveTeams = 0;
      for (int i = 0; i < entCount; i++) if (ents[i].alive) aliveTeams++;
      if (aliveTeams == 1 && ents[humanIdx].alive) matchResult = 1;
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
    else matchResult = ffa ? (ents[humanIdx].alive ? 1 : -1) : 0;
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
static void drawDigit(int x, int y, int d, KDColor c) {
  for (int r = 0; r < 5; r++) {
    uint8_t row = FONT5[d][r];
    for (int col = 0; col < 4; col++)
      if (row & (0x10 >> col)) { px(x + col, y + r, c); px(x + col + 1, y + r + 1, C_BLK); }
  }
}
static void drawNum(int x, int y, int v, KDColor c) {
  if (v < 0) { fRect(x, y + 2, 3, 1, c); drawNum(x + 5, y, -v, c); return; }
  char buf[8]; int n = 0;
  do { buf[n++] = '0' + v % 10; v /= 10; } while (v);
  for (int i = n - 1; i >= 0; i--) { drawDigit(x, y, buf[i] - '0', c); x += 6; }
}
static void drawText(int x, int y, const char *s, KDColor c) {
  while (*s) {
    char ch = *s++;
    if (ch == ' ') { x += 5; continue; }
    if (ch == '-') { fRect(x, y + 2, 4, 1, c); x += 5; continue; }
    if (ch == '!') { fRect(x, y, 1, 5, c); x += 4; continue; }
    if (ch == ':') { fRect(x, y, 1, 1, c); fRect(x, y + 3, 1, 1, c); x += 4; continue; }
    if (ch == '.') { fRect(x, y + 4, 1, 1, c); x += 4; continue; }
    if (ch == '+') { fRect(x, y + 2, 4, 1, c); fRect(x + 1, y + 1, 1, 3, c); x += 6; continue; }
    if (ch >= 'A' && ch <= 'Z') {
      for (int r = 0; r < 5; r++) {
        uint8_t row = FONTA[ch - 'A'][r];
        for (int col = 0; col < 5; col++)
          if (row & (0x10 >> col)) { px(x + col, y + r, c); px(x + col + 1, y + r + 1, C_BLK); }
      }
    } else if (ch >= '0' && ch <= '9') drawDigit(x, y, ch - '0', c);
    x += 6;
  }
}
static int textW(const char *s) { int w = 0; for (const char *p = s; *p; p++) w += (*p == ' ') ? 5 : 6; return w; }
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

  shadow(sx, sy + 7, 7, 3);
  if (e.hyperT > 0) { fRing(sx, sy, 12, C_MAG); fRing(sx, sy, 13 + (animT / 80) % 2, C_PUR); }
  int bob = ((e.walkT / 40) % 2) * (e.dashT > 0 ? 2 : 1);
  int by = sy - bob;
  drawBrawlerSprite(sx, by, 7, e.brawlerIdx, (int)e.faceAng, flash);
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

  for (int ty = camY / TILE; ty <= (camY + SH) / TILE && ty < MAPH; ty++)
    for (int tx = camX / TILE; tx <= (camX + SW) / TILE && tx < MAPW; tx++) {
      KDColor c = ((tx + ty) & 1) ? KDColor::RGB24(T.gr2) : KDColor::RGB24(T.gr1);
      fRect(tx * TILE - camX, ty * TILE - camY, TILE, TILE, c);
      drawGroundDecor(tx * TILE - camX, ty * TILE - camY, tx, ty);
      if (tileMap[tx][ty] == 1) drawWallThemed(tx * TILE - camX, ty * TILE - camY);
      else if (tileMap[tx][ty] == 2) drawBushThemed(tx * TILE - camX, ty * TILE - camY);
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

  // ---- HUD ----
  fRect(0, 0, SW, 24, C_PANEL);
  fRect(0, 22, SW, 2, C_PANEL2);
  const BrawlerDef &d = BRAWLERS[me.brawlerIdx];
  drawBrawlerSprite(16, 12, 8, me.brawlerIdx, 90, false);
  drawText(30, 5, d.name, C_WHT);
  int lvl = (me.accIdx >= 0) ? acc.b[me.accIdx].level : 1;
  fRect(30 + textW(d.name) + 2, 3, 14, 12, C_YEL);
  fRect(30 + textW(d.name) + 2, 12, 14, 3, C_BTND);
  drawNum(34 + textW(d.name), 5, lvl, C_BLK);
  fRect(150, 3, 18, 18, me.gadgetUses > 0 ? C_CYN : KDColor::RGB24(0x225566));
  drawText(155, 7, "X", C_BLK);
  drawNum(163, 7, me.gadgetUses, C_WHT);
  if (me.accIdx >= 0 && acc.b[me.accIdx].hyper) {
    fRect(172, 3, 24, 18, me.hyperT > 0 ? C_MAG : KDColor::RGB24(0x66114A));
    drawText(175, 8, "HYP", C_WHT);
  }
  drawNum(210, 8, fmaxf(0, 150 - matchTimeMs / 1000), C_WHT);

  fRect(240, 3, 76, 18, C_BLK);
  if (mode == MODE_GEM) {
    drawNum(250, 8, teamGems[0], C_BLUE);
    drawGem(270, 12, animT);
    drawNum(295, 8, teamGems[1], C_RED);
    if (gemCountdown > 0) drawNum(226, 8, gemCountdown / 1000, C_YEL);
  } else if (mode == MODE_BALL) {
    int g0 = 0, g1 = 0;
    for (int i = 0; i < entCount; i++)
      if (ents[i].team == 0) g0 = fmaxf(g0, ents[i].goals);
      else g1 = fmaxf(g1, ents[i].goals);
    drawNum(255, 8, g0, C_BLUE); drawBall(278, 12); drawNum(300, 8, g1, C_RED);
  } else if (mode == MODE_BOUNTY) {
    drawNum(252, 8, teamStars[0], C_GOLD);
    drawText(262, 8, "E", C_WHT);
    drawNum(298, 8, teamStars[1], C_RED);
  } else if (mode == MODE_HEIST) {
    drawNum(246, 8, (int)(safes[0].hp / 1000), C_BLUE);
    drawText(270, 8, "SAFE", C_GRE);
    drawNum(300, 8, (int)(safes[1].hp / 1000), C_RED);
  } else if (mode == MODE_ZONE) {
    int m = 0;
    for (int z = 0; z < MAX_ZONE; z++) if (zones[z].prog > 0) m++;
    drawText(246, 8, "ZONES", C_WHT); drawNum(300, 8, m, C_GREEN);
  } else if (mode == MODE_KO) {
    int a0 = 0, a1 = 0;
    for (int i = 0; i < entCount; i++)
      if (ents[i].alive) { if (ents[i].team == 0) a0++; else a1++; }
    drawNum(255, 8, a0, C_BLUE); drawText(272, 8, "VS", C_WHT); drawNum(300, 8, a1, C_RED);
  } else {
    int na = 0;
    for (int i = 0; i < entCount; i++) if (ents[i].alive) na++;
    drawText(246, 8, "VIVANTS", C_WHT); drawNum(300, 8, na, C_YEL);
  }

  // bandeau theme de la carte (coin bas gauche)
  fRect(0, SH - 12, SW, 12, KDColor::RGB24(0x111827));
  drawText(4, SH - 10, T.themeName, C_GRE);
  drawText(140, SH - 10, "OK ATT SHIFT SUP X GAD - HYP", C_GRE);
  blit();
}

// ------------------------- Menus --------------------------------------
static int menuPick(const char *title, const char **items, int n, int start) {
  int sel = start;
  while (true) {
    for (int y = 0; y < SH; y++)
      for (int x = 0; x < SW; x++)
        px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);
    drawTextC(6, title, C_YEL);
    fRect(SW / 2 - 60, 20, 120, 3, C_BTND);
    int first = (sel > 5) ? sel - 5 : 0;
    if (first > n - 11) first = fmaxf(0, n - 11);
    for (int i = first; i < n && i < first + 11; i++) {
      int y = 30 + (i - first) * 15;
      if (i == sel) rRect(20, y - 3, SW - 40, 13, 5, C_YEL);
      drawText(30, y, items[i], i == sel ? C_BLK : C_WHT);
    }
    drawTextC(SH - 14, "OK CHOISIR   BACK RETOUR", C_GRE);
    blit();
    pollKeys();
    if (kUp && sel > 0) sel--;
    if (kDown && sel < n - 1) sel++;
    if (keyOK && !prevOK) { prevOK = true; return sel; }
    if (keyBack && !prevBack) { prevBack = true; return -1; }
    prevOK = keyOK; prevBack = keyBack;
    Ion::Timing::msleep(60);
  }
}

static void trophyRoadScreen() {
  while (true) {
    for (int y = 0; y < SH; y++)
      for (int x = 0; x < SW; x++)
        px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);
    drawTextC(6, "ROUTE DES TROPHEES", C_YEL);
    fRect(SW / 2 - 60, 20, 120, 3, C_BTND);
    int tt = totalTrophies();
    drawText(20, 30, "TOTAL:", C_WHT);
    fRect(80, 28, 8, 4, C_GOLD); fRect(81, 32, 6, 2, C_GOLD); fRect(83, 34, 2, 4, C_GOLD);
    drawNum(96, 30, tt, C_YEL);
    drawText(20, 44, "BRAWLERS:", C_WHT);
    drawNum(96, 44, unlockedBrawlers, C_GREEN);
    drawText(110, 44, "SUR 50", C_GRE);
    drawText(20, 58, "PIECES:", C_WHT);
    oCirc(84, 60, 4, C_YEL, C_BTND);
    drawNum(96, 58, acc.coins, C_YEL);

    int next = -1;
    for (int i = 0; i < 50; i++)
      if (!isUnlocked(i)) { next = i; break; }
    drawTextC(76, "PROCHAIN DEBLOCAGE", C_WHT);
    if (next >= 0) {
      const BrawlerDef &d = BRAWLERS[next];
      rRect(SW / 2 - 110, 88, 220, 60, 8, KDColor::RGB24(RAR_COL[d.rar]));
      drawBrawlerSprite(SW / 2 - 80, 118, 16, next, 90, false);
      drawText(SW / 2 - 55, 102, d.name, C_WHT);
      drawText(SW / 2 - 55, 112, RAR_NAME[d.rar], KDColor::RGB24(RAR_COL[d.rar]));
      drawText(SW / 2 - 55, 124, "A", C_WHT);
      drawNum(SW / 2 - 43, 124, d.unlockTroph, C_YEL);
      drawText(SW / 2 - 55, 134, "TROPHEES", C_GRE);
      int prog = fminf(1.0f, (float)tt / d.unlockTroph);
      fRect(SW / 2 - 55, 148, 160, 6, C_BLK);
      fRect(SW / 2 - 54, 149, (int)(158 * prog), 4, C_GREEN);
    } else {
      drawTextC(110, "TOUT EST DEBLOQUE!", C_GOLD);
    }
    drawTextC(SH - 14, "OK RETOUR", C_GRE);
    blit();
    prevOK = true;
    while (prevOK) { pollKeys(); prevOK = keyOK; Ion::Timing::msleep(60); }
    return;
  }
}

static void brawlerMenu(int myBrawler) {
  while (true) {
    const char *names[50];
    for (int i = 0; i < 50; i++) names[i] = BRAWLERS[i].name;
    int sel = menuPick("CHOISIS TON BRAWLER", names, 50, myBrawler);
    if (sel < 0) return;
    myBrawler = sel;
    while (true) {
      const BrawlerDef &d = BRAWLERS[sel];
      BrawlerAccount &a = acc.b[sel];
      bool unlocked = isUnlocked(sel);
      float mul = lvlMul(a.level);
      for (int y = 0; y < SH; y++)
        for (int x = 0; x < SW; x++)
          px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);

      int bob = (animT / 400) % 2;
      rRect(18, 26, 62, 70, 8, unlocked ? KDColor::RGB24(RAR_COL[d.rar]) : C_DGREY);
      if (unlocked) {
        drawBrawlerSprite(50, 60 - bob, 22, sel, 90, false);
        fRect(64, 40, 22, 16, C_YEL); fRect(64, 51, 22, 5, C_BTND);
        drawNum(70, 43, a.level, C_BLK);
      } else {
        shCircRGB(50, 60 - bob, 22, 0x444444, 0x222222, C_GRE);
        drawText(40, 56, "?", C_GRE);
        drawText(22, 98, "A", C_GRE);
        drawNum(30, 98, d.unlockTroph, C_YEL);
        drawText(38, 98, "TROPHEES", C_GRE);
      }
      shadow(50, 88, 16, 4);
      drawText(90, 30, d.name, unlocked ? C_YEL : C_GRE);
      drawText(90, 42, RAR_NAME[d.rar], unlocked ? KDColor::RGB24(RAR_COL[d.rar]) : C_GRE);
      fRect(230, 28, 8, 4, C_GOLD); fRect(231, 32, 6, 2, C_GOLD); fRect(233, 34, 2, 4, C_GOLD);
      drawNum(245, 30, a.trophies, C_WHT);
      oCirc(230, 50, 4, C_YEL, C_BTND); drawNum(245, 46, acc.coins, C_WHT);

      if (unlocked) {
        rRect(10, 100, 90, 26, 5, KDColor::RGB24(0x2B3A55));
        drawText(16, 104, "PV", C_WHT);
        drawNum(16, 112, (int)(d.baseHP * mul * ((a.starPower && d.star == STAR_HP) ? 1.05f : 1)), C_GREEN);
        rRect(110, 100, 90, 26, 5, KDColor::RGB24(0x2B3A55));
        drawText(116, 104, "DEG", C_WHT);
        drawNum(116, 112, (int)(d.baseDmg * mul * ((a.starPower && d.star == STAR_DMG) ? 1.1f : 1)), C_ORG);
        rRect(210, 100, 100, 26, 5, KDColor::RGB24(0x2B3A55));
        drawText(216, 104, "PORTEE", C_WHT);
        drawNum(216, 112, d.range, C_CYN);

        rRect(10, 134, 96, 26, 5, a.gadget ? C_CYN : KDColor::RGB24(0x225566));
        drawText(16, 140, "GADGET", a.gadget ? C_BLK : C_GRE);
        drawText(16, 148, a.gadget ? "OUI" : "NON", a.gadget ? C_BLK : C_GRE);
        rRect(112, 134, 96, 26, 5, a.starPower ? C_PUR : KDColor::RGB24(0x3A1F55));
        drawText(118, 140, "POUV. STAR", a.starPower ? C_WHT : C_GRE);
        drawText(118, 148, a.starPower ? "OUI" : "NON", a.starPower ? C_WHT : C_GRE);
        rRect(214, 134, 96, 26, 5, a.hyper ? C_MAG : KDColor::RGB24(0x55114A));
        drawText(220, 140, "HYPERCH.", a.hyper ? C_WHT : C_GRE);
        drawText(220, 148, a.hyper ? "OUI" : (a.level >= 11 ? "DISPO" : "NIV 11"), a.hyper ? C_WHT : C_GRE);

        int cost = upgradeCost(a.level);
        if (a.level < 11) {
          rRect(10, 168, 140, 24, 5, (acc.coins >= cost) ? C_YEL : KDColor::RGB24(0x5A4A9A));
          drawText(16, 175, "+ NIV SUIVANT", C_BLK); drawNum(110, 175, cost, C_BLK);
        } else {
          rRect(10, 168, 140, 24, 5, C_GREEN);
          drawText(16, 175, "NIVEAU MAX!", C_BLK);
        }
        if (!a.gadget) { rRect(156, 168, 74, 24, 5, (acc.coins >= 100) ? C_CYN : KDColor::RGB24(0x225566)); drawText(162, 175, "* GADGET", C_BLK); drawNum(202, 175, 100, C_BLK); }
        if (a.level >= 3 && !a.starPower) { rRect(156, 196, 74, 22, 5, (acc.coins >= 150) ? C_PUR : KDColor::RGB24(0x3A1F55)); drawText(162, 202, "/ STAR", C_WHT); drawNum(202, 202, 150, C_WHT); }
        if (a.level >= 11 && !a.hyper) { rRect(234, 196, 76, 22, 5, (acc.coins >= 300) ? C_MAG : KDColor::RGB24(0x55114A)); drawText(240, 202, "- HYP", C_WHT); drawNum(272, 202, 300, C_WHT); }
        drawText(240, 175, "OK RETOUR", C_GRE);
      } else {
        drawTextC(120, "BRAWLER NON DEBLOQUE", C_GRE);
        drawTextC(140, "GAGNE DES TROPHEES POUR", C_WHT);
        drawTextC(152, "LE DEBLOQUER!", C_WHT);
      }
      blit();
      pollKeys();
      animT += 80;
      if (unlocked) {
        if (a.level < 11 && acc.coins >= cost && (Ion::Keyboard::getState().keyDown(Ion::Keyboard::Key::Plus)))
          { acc.coins -= cost; a.level++; }
        if (!a.gadget && acc.coins >= 100 && (Ion::Keyboard::getState().keyDown(Ion::Keyboard::Key::Multiply)))
          { acc.coins -= 100; a.gadget = true; }
        if (a.level >= 3 && !a.starPower && acc.coins >= 150 && (Ion::Keyboard::getState().keyDown(Ion::Keyboard::Key::Division)))
          { acc.coins -= 150; a.starPower = true; }
        if (a.level >= 11 && !a.hyper && acc.coins >= 300 && (Ion::Keyboard::getState().keyDown(Ion::Keyboard::Key::Power)))
          { acc.coins -= 300; a.hyper = true; }
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
  acc.coins += coins + starDrop;
  acc.b[myBrawler].trophies += troph;
  if (acc.b[myBrawler].trophies < 0) acc.b[myBrawler].trophies = 0;
  int newUnlock = -1;
  refreshUnlocks(&newUnlock);

  const char *msg = (result == 1) ? "VICTOIRE!" : (result == 0) ? "EGALITE" : "DEFAITE";
  KDColor col = (result == 1) ? C_YEL : (result == 0) ? C_WHT : C_RED;
  while (true) {
    for (int y = 0; y < SH; y++)
      for (int x = 0; x < SW; x++)
        px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);
    rRect(SW / 2 - 100, 20, 200, 42, 10, col);
    drawTextC(34, msg, C_BLK);
    oCirc(SW / 2 - 30, 84, 8, C_YEL, C_BTND);
    drawText(SW / 2 - 44, 80, "+", C_WHT);
    drawNum(SW / 2 - 16, 80, coins, C_WHT);
    fRect(SW / 2 + 20, 76, 10, 5, C_GOLD); fRect(SW / 2 + 22, 81, 6, 2, C_GOLD); fRect(SW / 2 + 24, 83, 2, 5, C_GOLD);
    drawNum(SW / 2 + 36, 80, troph, troph >= 0 ? C_YEL : C_RED);
    if (starDrop > 0) {
      drawTextC(104, "STAR DROP!", C_MAG);
      oCirc(SW / 2 - 40, 122, 6, C_MAG, C_WHT);
      drawText(SW / 2 - 24, 118, "+", C_WHT);
      drawNum(SW / 2 - 16, 118, starDrop, C_CYN);
      drawTextC(130, "PIECES BONUS", C_CYN);
    }
    if (newUnlock >= 0) {
      const BrawlerDef &d = BRAWLERS[newUnlock];
      rRect(SW / 2 - 110, 148, 220, 34, 8, KDColor::RGB24(RAR_COL[d.rar]));
      drawTextC(152, "NOUVEAU BRAWLER!", C_BLK);
      drawTextC(164, d.name, C_WHT);
    }
    rRect(SW / 2 - 70, 190, 140, 24, 8, C_YEL);
    drawTextC(196, "OK CONTINUER", C_BLK);
    blit();
    pollKeys();
    if (keyOK && !prevOK) { prevOK = true; return; }
    prevOK = keyOK;
    Ion::Timing::msleep(80);
  }
}

static void playMatch(int myBrawler, int selMode) {
  setupMatch(selMode, myBrawler);
  prevOK = prevShift = prevX = prevMinus = prevBack = false; keyBack = false; matchResult = -2;
  while (matchResult == -2) {
    int t0 = (int)Ion::Timing::millis();
    updateMatch(1.0f / 30.0f);
    renderMatch();
    int frameMs = (int)Ion::Timing::millis() - t0;
    Ion::Timing::msleep(frameMs < 33 ? 33 - frameMs : 1);
  }
  resultScreen(matchResult, myBrawler);
}

// ------------------------- Point d'entree ------------------------------
void ion_main(int argc, char *argv[]) {
  (void)argc; (void)argv;
  initAcc();
  int myBrawler = 0;
  const char *mainItems[4] = {"JOUER", "BRAWLERS", "ROUTE DES TROPHEES", "AIDE"};
  while (true) {
    int choice = menuPick("BRAWL MICROSTARS", mainItems, 4, 0);
    if (choice < 0) return;
    if (choice == 0) {
      const char *modes[MODE_COUNT];
      for (int i = 0; i < MODE_COUNT; i++) modes[i] = MODE_NAMES[i];
      int selMode = menuPick("MODE DE JEU", modes, MODE_COUNT, 0);
      if (selMode < 0) continue;
      while (true) {
        for (int y = 0; y < SH; y++)
          for (int x = 0; x < SW; x++)
            px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);
        drawTextC(20, MODE_NAMES[selMode], C_YEL);
        // apercu du theme de la carte
        const MapTheme &T = THEMES[selMode];
        rRect(SW / 2 - 100, 40, 200, 30, 6, KDColor::RGB24(T.gr1));
        fRect(SW / 2 - 90, 55, 20, 12, KDColor::RGB24(T.wall));
        fRect(SW / 2 - 60, 55, 20, 12, KDColor::RGB24(T.bush1));
        drawTextC(44, T.themeName, C_WHT);
        drawTextC(80, MODE_DESC[selMode], C_WHT);
        rRect(SW / 2 - 70, 100, 140, 24, 8, C_YEL);
        drawTextC(106, "OK JOUER", C_BLK);
        drawTextC(130, "BACK RETOUR", C_GRE);
        blit();
        pollKeys();
        if (keyOK && !prevOK) { prevOK = true; break; }
        if (keyBack && !prevBack) { prevBack = true; break; }
        prevOK = keyOK; prevBack = keyBack;
        Ion::Timing::msleep(60);
      }
      if (keyBack) { prevBack = false; continue; }
      playMatch(myBrawler, selMode);
    } else if (choice == 1) {
      brawlerMenu(myBrawler);
    } else if (choice == 2) {
      trophyRoadScreen();
    } else {
      for (int y = 0; y < SH; y++)
        for (int x = 0; x < SW; x++)
          px(x, y, (((x + y * 2) / 40) % 2) ? C_PANEL : C_PANEL2);
      drawTextC(8, "AIDE", C_YEL);
      fRect(SW / 2 - 60, 22, 120, 3, C_BTND);
      const char *lines[8] = {
        "FLECHES: BOUGER", "OK: ATTAQUE (AUTO-VISEE)",
        "SHIFT: SUPER", "X: GADGET", "-: HYPERCHARGE",
        "+ * /: ACHATS MENU BRAWLERS", "GAGNE DES TROPHEES POUR", "DEBLOQUER DES BRAWLERS"
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
