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
#ifndef _UAPI_LINUX_FD_H
#define _UAPI_LINUX_FD_H
#include <linux/ioctl.h>
#include <linux/compiler.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct floppy_struct {
  unsigned int size, sect, head, track, stretch;
#define FD_STRETCH 1
#define FD_SWAPSIDES 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_ZEROBASED 4
#define FD_SECTBASEMASK 0x3FC
#define FD_MKSECTBASE(s) (((s) ^ 1) << 2)
#define FD_SECTBASE(floppy) ((((floppy)->stretch & FD_SECTBASEMASK) >> 2) ^ 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char gap, rate,
#define FD_2M 0x4
#define FD_SIZECODEMASK 0x38
#define FD_SIZECODE(floppy) (((((floppy)->rate & FD_SIZECODEMASK) >> 3) + 2) % 8)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_SECTSIZE(floppy) ((floppy)->rate & FD_2M ? 512 : 128 << FD_SIZECODE(floppy))
#define FD_PERP 0x40
  spec1, fmt_gap;
  const char * name;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define FDCLRPRM _IO(2, 0x41)
#define FDSETPRM _IOW(2, 0x42, struct floppy_struct)
#define FDSETMEDIAPRM FDSETPRM
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDDEFPRM _IOW(2, 0x43, struct floppy_struct)
#define FDGETPRM _IOR(2, 0x04, struct floppy_struct)
#define FDDEFMEDIAPRM FDDEFPRM
#define FDGETMEDIAPRM FDGETPRM
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDMSGON _IO(2, 0x45)
#define FDMSGOFF _IO(2, 0x46)
#define FD_FILL_BYTE 0xF6
struct format_descr {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int device, head, track;
};
#define FDFMTBEG _IO(2, 0x47)
#define FDFMTTRK _IOW(2, 0x48, struct format_descr)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDFMTEND _IO(2, 0x49)
struct floppy_max_errors {
  unsigned int abort, read_track, reset, recal, reporting;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDSETEMSGTRESH _IO(2, 0x4a)
#define FDFLUSH _IO(2, 0x4b)
#define FDSETMAXERRS _IOW(2, 0x4c, struct floppy_max_errors)
#define FDGETMAXERRS _IOR(2, 0x0e, struct floppy_max_errors)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef char floppy_drive_name[16];
#define FDGETDRVTYP _IOR(2, 0x0f, floppy_drive_name)
struct floppy_drive_params {
  signed char cmos;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned long max_dtr;
  unsigned long hlt;
  unsigned long hut;
  unsigned long srt;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned long spinup;
  unsigned long spindown;
  unsigned char spindown_offset;
  unsigned char select_delay;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char rps;
  unsigned char tracks;
  unsigned long timeout;
  unsigned char interleave_sect;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct floppy_max_errors max_errors;
  char flags;
#define FTD_MSG 0x10
#define FD_BROKEN_DCL 0x20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_DEBUG 0x02
#define FD_SILENT_DCL_CLEAR 0x4
#define FD_INVERTED_DCL 0x80
  char read_track;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  short autodetect[8];
  int checkfreq;
  int native_format;
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum {
  FD_NEED_TWADDLE_BIT,
  FD_VERIFY_BIT,
  FD_DISK_NEWCHANGE_BIT,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FD_UNUSED_BIT,
  FD_DISK_CHANGED_BIT,
  FD_DISK_WRITABLE_BIT,
  FD_OPEN_SHOULD_FAIL_BIT
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define FDSETDRVPRM _IOW(2, 0x90, struct floppy_drive_params)
#define FDGETDRVPRM _IOR(2, 0x11, struct floppy_drive_params)
struct floppy_drive_struct {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned long flags;
#define FD_NEED_TWADDLE (1 << FD_NEED_TWADDLE_BIT)
#define FD_VERIFY (1 << FD_VERIFY_BIT)
#define FD_DISK_NEWCHANGE (1 << FD_DISK_NEWCHANGE_BIT)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_DISK_CHANGED (1 << FD_DISK_CHANGED_BIT)
#define FD_DISK_WRITABLE (1 << FD_DISK_WRITABLE_BIT)
  unsigned long spinup_date;
  unsigned long select_date;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned long first_read_date;
  short probed_format;
  short track;
  short maxblock;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  short maxtrack;
  int generation;
  int keep_data;
  int fd_ref;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int fd_device;
  unsigned long last_checked;
  char * dmabuf;
  int bufblocks;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define FDGETDRVSTAT _IOR(2, 0x12, struct floppy_drive_struct)
#define FDPOLLDRVSTAT _IOR(2, 0x13, struct floppy_drive_struct)
enum reset_mode {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  FD_RESET_IF_NEEDED,
  FD_RESET_IF_RAWCMD,
  FD_RESET_ALWAYS
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDRESET _IO(2, 0x54)
struct floppy_fdc_state {
  int spec1;
  int spec2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int dtr;
  unsigned char version;
  unsigned char dor;
  unsigned long address;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int rawcmd : 2;
  unsigned int reset : 1;
  unsigned int need_configure : 1;
  unsigned int perp_mode : 2;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int has_fifo : 1;
  unsigned int driver_version;
#define FD_DRIVER_VERSION 0x100
  unsigned char track[4];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
#define FDGETFDCSTAT _IOR(2, 0x15, struct floppy_fdc_state)
struct floppy_write_errors {
  unsigned int write_errors;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned long first_error_sector;
  int first_error_generation;
  unsigned long last_error_sector;
  int last_error_generation;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int badness;
};
#define FDWERRORCLR _IO(2, 0x56)
#define FDWERRORGET _IOR(2, 0x17, struct floppy_write_errors)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDHAVEBATCHEDRAWCMD
struct floppy_raw_cmd {
  unsigned int flags;
#define FD_RAW_READ 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_RAW_WRITE 2
#define FD_RAW_NO_MOTOR 4
#define FD_RAW_DISK_CHANGE 4
#define FD_RAW_INTR 8
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_RAW_SPIN 0x10
#define FD_RAW_NO_MOTOR_AFTER 0x20
#define FD_RAW_NEED_DISK 0x40
#define FD_RAW_NEED_SEEK 0x80
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_RAW_MORE 0x100
#define FD_RAW_STOP_IF_FAILURE 0x200
#define FD_RAW_STOP_IF_SUCCESS 0x400
#define FD_RAW_SOFTFAILURE 0x800
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FD_RAW_FAILURE 0x10000
#define FD_RAW_HARDFAILURE 0x20000
  void __user * data;
  char * kernel_data;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  struct floppy_raw_cmd * next;
  long length;
  long phys_length;
  int buffer_length;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char rate;
  unsigned char cmd_count;
  unsigned char cmd[16];
  unsigned char reply_count;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned char reply[16];
  int track;
  int resultcode;
  int reserved1;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  int reserved2;
};
#define FDRAWCMD _IO(2, 0x58)
#define FDTWADDLE _IO(2, 0x59)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define FDEJECT _IO(2, 0x5a)
#endif
