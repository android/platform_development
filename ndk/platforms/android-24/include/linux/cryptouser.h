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
enum {
  CRYPTO_MSG_BASE = 0x10,
  CRYPTO_MSG_NEWALG = 0x10,
  CRYPTO_MSG_DELALG,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CRYPTO_MSG_UPDATEALG,
  CRYPTO_MSG_GETALG,
  CRYPTO_MSG_DELRNG,
  __CRYPTO_MSG_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define CRYPTO_MSG_MAX (__CRYPTO_MSG_MAX - 1)
#define CRYPTO_NR_MSGTYPES (CRYPTO_MSG_MAX + 1 - CRYPTO_MSG_BASE)
#define CRYPTO_MAX_NAME CRYPTO_MAX_ALG_NAME
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum crypto_attr_type_t {
  CRYPTOCFGA_UNSPEC,
  CRYPTOCFGA_PRIORITY_VAL,
  CRYPTOCFGA_REPORT_LARVAL,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CRYPTOCFGA_REPORT_HASH,
  CRYPTOCFGA_REPORT_BLKCIPHER,
  CRYPTOCFGA_REPORT_AEAD,
  CRYPTOCFGA_REPORT_COMPRESS,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  CRYPTOCFGA_REPORT_RNG,
  CRYPTOCFGA_REPORT_CIPHER,
  CRYPTOCFGA_REPORT_AKCIPHER,
  __CRYPTOCFGA_MAX
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CRYPTOCFGA_MAX (__CRYPTOCFGA_MAX - 1)
};
struct crypto_user_alg {
  char cru_name[CRYPTO_MAX_ALG_NAME];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char cru_driver_name[CRYPTO_MAX_ALG_NAME];
  char cru_module_name[CRYPTO_MAX_ALG_NAME];
  __u32 cru_type;
  __u32 cru_mask;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  __u32 cru_refcnt;
  __u32 cru_flags;
};
struct crypto_report_larval {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char type[CRYPTO_MAX_NAME];
};
struct crypto_report_hash {
  char type[CRYPTO_MAX_NAME];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int blocksize;
  unsigned int digestsize;
};
struct crypto_report_cipher {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char type[CRYPTO_MAX_ALG_NAME];
  unsigned int blocksize;
  unsigned int min_keysize;
  unsigned int max_keysize;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct crypto_report_blkcipher {
  char type[CRYPTO_MAX_NAME];
  char geniv[CRYPTO_MAX_NAME];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int blocksize;
  unsigned int min_keysize;
  unsigned int max_keysize;
  unsigned int ivsize;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct crypto_report_aead {
  char type[CRYPTO_MAX_NAME];
  char geniv[CRYPTO_MAX_NAME];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int blocksize;
  unsigned int maxauthsize;
  unsigned int ivsize;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct crypto_report_comp {
  char type[CRYPTO_MAX_NAME];
};
struct crypto_report_rng {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char type[CRYPTO_MAX_NAME];
  unsigned int seedsize;
};
struct crypto_report_akcipher {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char type[CRYPTO_MAX_NAME];
};
#define CRYPTO_REPORT_MAXSIZE (sizeof(struct crypto_user_alg) + sizeof(struct crypto_report_blkcipher))
