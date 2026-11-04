#ifndef MBOX_IOCTL_H
#define MBOX_IOCTL_H
/* Include ntddk.h in the driver, or windows.h + winioctl.h in user mode. */
#define IOCTL_MBOX_SET_RATE CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA)
#define IOCTL_MBOX_TRANSFER CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA)
#define MBOX_ABI_VERSION 1u
#define MBOX_PACKET_COUNT 8u
#define MBOX_BUFFER_BYTES (MBOX_PACKET_COUNT*304u)
/* Direction: 0 playback, 1 capture. Playback lengths must be complete stereo24
 * frames; capture ignores lengths and reserves 304 bytes per interval. */
typedef struct {
 ULONG Version,Direction;
 ULONG Length[MBOX_PACKET_COUNT];
 UCHAR Data[MBOX_BUFFER_BYTES];
} MBOX_TRANSFER_INPUT;
typedef struct {
 ULONG Version,UsbStatus,ErrorCount,StartFrame;
 ULONG Offset[MBOX_PACKET_COUNT],Length[MBOX_PACKET_COUNT],Status[MBOX_PACKET_COUNT];
 UCHAR Data[MBOX_BUFFER_BYTES];
} MBOX_TRANSFER_OUTPUT;
#endif
