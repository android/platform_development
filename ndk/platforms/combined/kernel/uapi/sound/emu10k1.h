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
#ifndef _UAPI__SOUND_EMU10K1_H
#define _UAPI__SOUND_EMU10K1_H
#include <linux/types.h>
#define EMU10K1_CARD_CREATIVE 0x00000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EMU10K1_CARD_EMUAPS 0x00000001
#define EMU10K1_FX8010_PCM_COUNT 8
#define iMAC0 0x00
#define iMAC1 0x01
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define iMAC2 0x02
#define iMAC3 0x03
#define iMACINT0 0x04
#define iMACINT1 0x05
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define iACC3 0x06
#define iMACMV 0x07
#define iANDXOR 0x08
#define iTSTNEG 0x09
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define iLIMITGE 0x0a
#define iLIMITLT 0x0b
#define iLOG 0x0c
#define iEXP 0x0d
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define iINTERP 0x0e
#define iSKIP 0x0f
#define FXBUS(x) (0x00 + (x))
#define EXTIN(x) (0x10 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTOUT(x) (0x20 + (x))
#define FXBUS2(x) (0x30 + (x))
#define C_00000000 0x40
#define C_00000001 0x41
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define C_00000002 0x42
#define C_00000003 0x43
#define C_00000004 0x44
#define C_00000008 0x45
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define C_00000010 0x46
#define C_00000020 0x47
#define C_00000100 0x48
#define C_00010000 0x49
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define C_00080000 0x4a
#define C_10000000 0x4b
#define C_20000000 0x4c
#define C_40000000 0x4d
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define C_80000000 0x4e
#define C_7fffffff 0x4f
#define C_ffffffff 0x50
#define C_fffffffe 0x51
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define C_c0000000 0x52
#define C_4f1bbcdc 0x53
#define C_5a7ef9db 0x54
#define C_00100000 0x55
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define GPR_ACCU 0x56
#define GPR_COND 0x57
#define GPR_NOISE0 0x58
#define GPR_NOISE1 0x59
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define GPR_IRQ 0x5a
#define GPR_DBAC 0x5b
#define GPR(x) (FXGPREGBASE + (x))
#define ITRAM_DATA(x) (TANKMEMDATAREGBASE + 0x00 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ETRAM_DATA(x) (TANKMEMDATAREGBASE + 0x80 + (x))
#define ITRAM_ADDR(x) (TANKMEMADDRREGBASE + 0x00 + (x))
#define ETRAM_ADDR(x) (TANKMEMADDRREGBASE + 0x80 + (x))
#define A_ITRAM_DATA(x) (TANKMEMDATAREGBASE + 0x00 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_ETRAM_DATA(x) (TANKMEMDATAREGBASE + 0xc0 + (x))
#define A_ITRAM_ADDR(x) (TANKMEMADDRREGBASE + 0x00 + (x))
#define A_ETRAM_ADDR(x) (TANKMEMADDRREGBASE + 0xc0 + (x))
#define A_ITRAM_CTL(x) (A_TANKMEMCTLREGBASE + 0x00 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_ETRAM_CTL(x) (A_TANKMEMCTLREGBASE + 0xc0 + (x))
#define A_FXBUS(x) (0x00 + (x))
#define A_EXTIN(x) (0x40 + (x))
#define A_P16VIN(x) (0x50 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT(x) (0x60 + (x))
#define A_FXBUS2(x) (0x80 + (x))
#define A_EMU32OUTH(x) (0xa0 + (x))
#define A_EMU32OUTL(x) (0xb0 + (x))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A3_EMU32IN(x) (0x160 + (x))
#define A3_EMU32OUT(x) (0x1E0 + (x))
#define A_GPR(x) (A_FXGPREGBASE + (x))
#define CC_REG_NORMALIZED C_00000001
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CC_REG_BORROW C_00000002
#define CC_REG_MINUS C_00000004
#define CC_REG_ZERO C_00000008
#define CC_REG_SATURATE C_00000010
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define CC_REG_NONZERO C_00000100
#define FXBUS_PCM_LEFT 0x00
#define FXBUS_PCM_RIGHT 0x01
#define FXBUS_PCM_LEFT_REAR 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FXBUS_PCM_RIGHT_REAR 0x03
#define FXBUS_MIDI_LEFT 0x04
#define FXBUS_MIDI_RIGHT 0x05
#define FXBUS_PCM_CENTER 0x06
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FXBUS_PCM_LFE 0x07
#define FXBUS_PCM_LEFT_FRONT 0x08
#define FXBUS_PCM_RIGHT_FRONT 0x09
#define FXBUS_MIDI_REVERB 0x0c
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FXBUS_MIDI_CHORUS 0x0d
#define FXBUS_PCM_LEFT_SIDE 0x0e
#define FXBUS_PCM_RIGHT_SIDE 0x0f
#define FXBUS_PT_LEFT 0x14
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FXBUS_PT_RIGHT 0x15
#define EXTIN_AC97_L 0x00
#define EXTIN_AC97_R 0x01
#define EXTIN_SPDIF_CD_L 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTIN_SPDIF_CD_R 0x03
#define EXTIN_ZOOM_L 0x04
#define EXTIN_ZOOM_R 0x05
#define EXTIN_TOSLINK_L 0x06
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTIN_TOSLINK_R 0x07
#define EXTIN_LINE1_L 0x08
#define EXTIN_LINE1_R 0x09
#define EXTIN_COAX_SPDIF_L 0x0a
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTIN_COAX_SPDIF_R 0x0b
#define EXTIN_LINE2_L 0x0c
#define EXTIN_LINE2_R 0x0d
#define EXTOUT_AC97_L 0x00
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTOUT_AC97_R 0x01
#define EXTOUT_TOSLINK_L 0x02
#define EXTOUT_TOSLINK_R 0x03
#define EXTOUT_AC97_CENTER 0x04
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTOUT_AC97_LFE 0x05
#define EXTOUT_HEADPHONE_L 0x06
#define EXTOUT_HEADPHONE_R 0x07
#define EXTOUT_REAR_L 0x08
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTOUT_REAR_R 0x09
#define EXTOUT_ADC_CAP_L 0x0a
#define EXTOUT_ADC_CAP_R 0x0b
#define EXTOUT_MIC_CAP 0x0c
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EXTOUT_AC97_REAR_L 0x0d
#define EXTOUT_AC97_REAR_R 0x0e
#define EXTOUT_ACENTER 0x11
#define EXTOUT_ALFE 0x12
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTIN_AC97_L 0x00
#define A_EXTIN_AC97_R 0x01
#define A_EXTIN_SPDIF_CD_L 0x02
#define A_EXTIN_SPDIF_CD_R 0x03
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTIN_OPT_SPDIF_L 0x04
#define A_EXTIN_OPT_SPDIF_R 0x05
#define A_EXTIN_LINE2_L 0x08
#define A_EXTIN_LINE2_R 0x09
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTIN_ADC_L 0x0a
#define A_EXTIN_ADC_R 0x0b
#define A_EXTIN_AUX2_L 0x0c
#define A_EXTIN_AUX2_R 0x0d
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_FRONT_L 0x00
#define A_EXTOUT_FRONT_R 0x01
#define A_EXTOUT_CENTER 0x02
#define A_EXTOUT_LFE 0x03
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_HEADPHONE_L 0x04
#define A_EXTOUT_HEADPHONE_R 0x05
#define A_EXTOUT_REAR_L 0x06
#define A_EXTOUT_REAR_R 0x07
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_AFRONT_L 0x08
#define A_EXTOUT_AFRONT_R 0x09
#define A_EXTOUT_ACENTER 0x0a
#define A_EXTOUT_ALFE 0x0b
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_ASIDE_L 0x0c
#define A_EXTOUT_ASIDE_R 0x0d
#define A_EXTOUT_AREAR_L 0x0e
#define A_EXTOUT_AREAR_R 0x0f
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_AC97_L 0x10
#define A_EXTOUT_AC97_R 0x11
#define A_EXTOUT_ADC_CAP_L 0x16
#define A_EXTOUT_ADC_CAP_R 0x17
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_EXTOUT_MIC_CAP 0x18
#define A_C_00000000 0xc0
#define A_C_00000001 0xc1
#define A_C_00000002 0xc2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_C_00000003 0xc3
#define A_C_00000004 0xc4
#define A_C_00000008 0xc5
#define A_C_00000010 0xc6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_C_00000020 0xc7
#define A_C_00000100 0xc8
#define A_C_00010000 0xc9
#define A_C_00000800 0xca
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_C_10000000 0xcb
#define A_C_20000000 0xcc
#define A_C_40000000 0xcd
#define A_C_80000000 0xce
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_C_7fffffff 0xcf
#define A_C_ffffffff 0xd0
#define A_C_fffffffe 0xd1
#define A_C_c0000000 0xd2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_C_4f1bbcdc 0xd3
#define A_C_5a7ef9db 0xd4
#define A_C_00100000 0xd5
#define A_GPR_ACCU 0xd6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_GPR_COND 0xd7
#define A_GPR_NOISE0 0xd8
#define A_GPR_NOISE1 0xd9
#define A_GPR_IRQ 0xda
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define A_GPR_DBAC 0xdb
#define A_GPR_DBACE 0xde
#define EMU10K1_DBG_ZC 0x80000000
#define EMU10K1_DBG_SATURATION_OCCURED 0x02000000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EMU10K1_DBG_SATURATION_ADDR 0x01ff0000
#define EMU10K1_DBG_SINGLE_STEP 0x00008000
#define EMU10K1_DBG_STEP 0x00004000
#define EMU10K1_DBG_CONDITION_CODE 0x00003e00
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EMU10K1_DBG_SINGLE_STEP_ADDR 0x000001ff
#define TANKMEMADDRREG_ADDR_MASK 0x000fffff
#define TANKMEMADDRREG_CLEAR 0x00800000
#define TANKMEMADDRREG_ALIGN 0x00400000
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TANKMEMADDRREG_WRITE 0x00200000
#define TANKMEMADDRREG_READ 0x00100000
struct snd_emu10k1_fx8010_info {
 unsigned int internal_tram_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int external_tram_size;
 char fxbus_names[16][32];
 char extin_names[16][32];
 char extout_names[32][32];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int gpr_controls;
};
#define EMU10K1_GPR_TRANSLATION_NONE 0
#define EMU10K1_GPR_TRANSLATION_TABLE100 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define EMU10K1_GPR_TRANSLATION_BASS 2
#define EMU10K1_GPR_TRANSLATION_TREBLE 3
#define EMU10K1_GPR_TRANSLATION_ONOFF 4
struct snd_emu10k1_fx8010_control_gpr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct snd_ctl_elem_id id;
 unsigned int vcount;
 unsigned int count;
 unsigned short gpr[32];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int value[32];
 unsigned int min;
 unsigned int max;
 unsigned int translation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 const unsigned int *tlv;
};
struct snd_emu10k1_fx8010_control_old_gpr {
 struct snd_ctl_elem_id id;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int vcount;
 unsigned int count;
 unsigned short gpr[32];
 unsigned int value[32];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int min;
 unsigned int max;
 unsigned int translation;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct snd_emu10k1_fx8010_code {
 char name[128];
 DECLARE_BITMAP(gpr_valid, 0x200);
 __u32 __user *gpr_map;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int gpr_add_control_count;
 struct snd_emu10k1_fx8010_control_gpr __user *gpr_add_controls;
 unsigned int gpr_del_control_count;
 struct snd_ctl_elem_id __user *gpr_del_controls;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int gpr_list_control_count;
 unsigned int gpr_list_control_total;
 struct snd_emu10k1_fx8010_control_gpr __user *gpr_list_controls;
 DECLARE_BITMAP(tram_valid, 0x100);
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 __user *tram_data_map;
 __u32 __user *tram_addr_map;
 DECLARE_BITMAP(code_valid, 1024);
 __u32 __user *code;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct snd_emu10k1_fx8010_tram {
 unsigned int address;
 unsigned int size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int *samples;
};
struct snd_emu10k1_fx8010_pcm_rec {
 unsigned int substream;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int res1;
 unsigned int channels;
 unsigned int tram_start;
 unsigned int buffer_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short gpr_size;
 unsigned short gpr_ptr;
 unsigned short gpr_count;
 unsigned short gpr_tmpcount;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned short gpr_trigger;
 unsigned short gpr_running;
 unsigned char pad;
 unsigned char etram[32];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 unsigned int res2;
};
#define SNDRV_EMU10K1_VERSION SNDRV_PROTOCOL_VERSION(1, 0, 1)
#define SNDRV_EMU10K1_IOCTL_INFO _IOR ('H', 0x10, struct snd_emu10k1_fx8010_info)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_EMU10K1_IOCTL_CODE_POKE _IOW ('H', 0x11, struct snd_emu10k1_fx8010_code)
#define SNDRV_EMU10K1_IOCTL_CODE_PEEK _IOWR('H', 0x12, struct snd_emu10k1_fx8010_code)
#define SNDRV_EMU10K1_IOCTL_TRAM_SETUP _IOW ('H', 0x20, int)
#define SNDRV_EMU10K1_IOCTL_TRAM_POKE _IOW ('H', 0x21, struct snd_emu10k1_fx8010_tram)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_EMU10K1_IOCTL_TRAM_PEEK _IOWR('H', 0x22, struct snd_emu10k1_fx8010_tram)
#define SNDRV_EMU10K1_IOCTL_PCM_POKE _IOW ('H', 0x30, struct snd_emu10k1_fx8010_pcm_rec)
#define SNDRV_EMU10K1_IOCTL_PCM_PEEK _IOWR('H', 0x31, struct snd_emu10k1_fx8010_pcm_rec)
#define SNDRV_EMU10K1_IOCTL_PVERSION _IOR ('H', 0x40, int)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_EMU10K1_IOCTL_STOP _IO ('H', 0x80)
#define SNDRV_EMU10K1_IOCTL_CONTINUE _IO ('H', 0x81)
#define SNDRV_EMU10K1_IOCTL_ZERO_TRAM_COUNTER _IO ('H', 0x82)
#define SNDRV_EMU10K1_IOCTL_SINGLE_STEP _IOW ('H', 0x83, int)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define SNDRV_EMU10K1_IOCTL_DBG_READ _IOR ('H', 0x84, int)
typedef struct snd_emu10k1_fx8010_info emu10k1_fx8010_info_t;
typedef struct snd_emu10k1_fx8010_control_gpr emu10k1_fx8010_control_gpr_t;
typedef struct snd_emu10k1_fx8010_code emu10k1_fx8010_code_t;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef struct snd_emu10k1_fx8010_tram emu10k1_fx8010_tram_t;
typedef struct snd_emu10k1_fx8010_pcm_rec emu10k1_fx8010_pcm_t;
#endif
