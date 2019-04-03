// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <windows.h>
#include <sstream>
#include <evntprov.h>
#include <memory>
#include <map>
#include <stdexcept>
#include <unordered_map>

#include "DenyCopyAssign.h"
#include "DenyCopyConstruct.h"
#include "DenyCopy.h"
#include "Error.h"
#include "UniquePtr.h"
#include "ReaderWriterLockSlim.h"
#include "AcquireReadLock.h"
#include "AcquireWriteLock.h"
#include "TracePointTypes.h"
#include "TracePointData.h"
#include "CriticalSection.h"
#include "AcquireCriticalSection.h"
#include "TracePointCommand.h"
#include "ServerEventSource.h"
#include "ClientEventSource.h"
#include "NamedPipeServer.h"
#include "NamedPipeClient.h"
#include "detours.h"
#include "DetourTransaction.h"
#include "DetourManager.h"
#include "EventMap.h"
#include "RegHandleMap.h"
#include "ProviderMap.h"
#include "TracePointModule.h"
#include "TracePointServer.h"
