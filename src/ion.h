#ifndef MINI_BS_ION_COMPAT_H
#define MINI_BS_ION_COMPAT_H

#include <stdint.h>
extern "C" {
#include <eadk.h> // garde extern "C" : evite que le C++ decore les noms des fonctions EADK
}

// Compatibility layer for the original Epsilon-internal Ion API used by the game.
class KDColor {
public:
  constexpr KDColor() : value(0) {}
  static constexpr KDColor RGB24(uint32_t rgb) {
    return KDColor((uint16_t)(((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F)));
  }
  static constexpr KDColor RGB(uint8_t r, uint8_t g, uint8_t b) {
    return KDColor((uint16_t)(((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (b >> 3)));
  }
  uint8_t red() const { return (uint8_t)((((value >> 11) & 31) * 255 + 15) / 31); }
  uint8_t green() const { return (uint8_t)((((value >> 5) & 63) * 255 + 31) / 63); }
  uint8_t blue() const { return (uint8_t)(((value & 31) * 255 + 15) / 31); }
  operator eadk_color_t() const { return (eadk_color_t)value; }
private:
  constexpr explicit KDColor(uint16_t v) : value(v) {}
  uint16_t value;
};
static_assert(sizeof(KDColor) == sizeof(eadk_color_t), "KDColor must match EADK color storage");

struct KDRect {
  uint16_t x, y, width, height;
  KDRect(int px, int py, int w, int h) : x(px), y(py), width(w), height(h) {}
};

namespace Ion {
namespace Display {
inline void pushRect(KDRect r, const KDColor *pixels) {
  eadk_rect_t rect = {r.x, r.y, r.width, r.height};
  eadk_display_push_rect(rect, reinterpret_cast<const eadk_color_t *>(pixels));
}
inline void pushRectUniform(KDRect r, KDColor c) {
  eadk_rect_t rect = {r.x, r.y, r.width, r.height};
  eadk_display_push_rect_uniform(rect, c);
}
}
namespace Keyboard {
enum class Key : uint8_t {
  Left = 0, Up = 1, Down = 2, Right = 3, OK = 4, Back = 5,
  Shift = 12, X = 14, Var = 15, Toolbox = 16, Exp = 18,
  Power = 23, Multiply = 39, Division = 40,
