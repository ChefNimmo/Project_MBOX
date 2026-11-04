// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "BridgeProtocol.h"
class CMiniportWaveRTStream;
NTSTATUS NimBridgeInitialize(PDRIVER_OBJECT);
void NimBridgeShutdown();
NTSTATUS NimBridgeRegister(CMiniportWaveRTStream*, BOOLEAN capture);
void NimBridgeUnregister(CMiniportWaveRTStream*);

