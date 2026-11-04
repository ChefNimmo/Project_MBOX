#pragma once
#include <windows.h>
#include "../kmdf/mbox_ioctl.h"
#include "../core/mbox_gate.h"
enum { SLOT_COUNT=2*MBOX_PIPE_DEPTH };
typedef struct {
 unsigned Index,Direction;int Active;OVERLAPPED Ov;
 MBOX_TRANSFER_INPUT Input;MBOX_TRANSFER_OUTPUT Output;
} SLOT;
typedef struct BACKEND {
 void *Context;
 int (*Submit)(struct BACKEND *,SLOT *);
 int (*Wait)(struct BACKEND *,SLOT *);
 int (*Finish)(struct BACKEND *,SLOT *,DWORD *);
 void (*Cancel)(struct BACKEND *,SLOT *);
 int (*Stop)(struct BACKEND *);
} BACKEND;
