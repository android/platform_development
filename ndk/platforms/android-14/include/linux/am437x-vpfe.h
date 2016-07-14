/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef AM437X_VPFE_USER_H
#define AM437X_VPFE_USER_H
#include <linux/videodev2.h>
enum vpfe_ccdc_data_size {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_DATA_16BITS = 0,
  VPFE_CCDC_DATA_15BITS,
  VPFE_CCDC_DATA_14BITS,
  VPFE_CCDC_DATA_13BITS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_DATA_12BITS,
  VPFE_CCDC_DATA_11BITS,
  VPFE_CCDC_DATA_10BITS,
  VPFE_CCDC_DATA_8BITS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
enum vpfe_ccdc_sample_length {
  VPFE_CCDC_SAMPLE_1PIXELS = 0,
  VPFE_CCDC_SAMPLE_2PIXELS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_SAMPLE_4PIXELS,
  VPFE_CCDC_SAMPLE_8PIXELS,
  VPFE_CCDC_SAMPLE_16PIXELS,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum vpfe_ccdc_sample_line {
  VPFE_CCDC_SAMPLE_1LINES = 0,
  VPFE_CCDC_SAMPLE_2LINES,
  VPFE_CCDC_SAMPLE_4LINES,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_SAMPLE_8LINES,
  VPFE_CCDC_SAMPLE_16LINES,
};
enum vpfe_ccdc_gamma_width {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_GAMMA_BITS_15_6 = 0,
  VPFE_CCDC_GAMMA_BITS_14_5,
  VPFE_CCDC_GAMMA_BITS_13_4,
  VPFE_CCDC_GAMMA_BITS_12_3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  VPFE_CCDC_GAMMA_BITS_11_2,
  VPFE_CCDC_GAMMA_BITS_10_1,
  VPFE_CCDC_GAMMA_BITS_09_0,
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct vpfe_ccdc_a_law {
  unsigned char enable;
  enum vpfe_ccdc_gamma_width gamma_wd;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct vpfe_ccdc_black_clamp {
  unsigned char enable;
  enum vpfe_ccdc_sample_length sample_pixel;
  enum vpfe_ccdc_sample_line sample_ln;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned short start_pixel;
  unsigned short sgain;
  unsigned short dc_sub;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct vpfe_ccdc_black_compensation {
  char r;
  char gr;
  char b;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char gb;
};
struct vpfe_ccdc_config_params_raw {
  enum vpfe_ccdc_data_size data_sz;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct vpfe_ccdc_a_law alaw;
  struct vpfe_ccdc_black_clamp blk_clamp;
  struct vpfe_ccdc_black_compensation blk_comp;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define VIDIOC_AM437X_CCDC_CFG _IOW('V', BASE_VIDIOC_PRIVATE + 1, void *)
#endif
