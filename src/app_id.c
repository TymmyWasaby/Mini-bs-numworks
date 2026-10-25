/* Identite NWA de l'app — compile en C sans LTO (cf. Makefile).
 * Le chargeur d'Epsilon exige ces sections nommees. */
#include <stdint.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "mini-bs";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
