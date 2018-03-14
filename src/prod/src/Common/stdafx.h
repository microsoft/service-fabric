// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Common/KtlSF.Common.h"
#include "Common/TimerEventSource.h"
#include "Common/GeneralEventSource.h"
#include "Common/ConfigEventSource.h"
#include "ServiceModel/SchemaNames.h"
#include <iomanip>
#include <regex>

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "CryptoUtility.Linux.h"
#include <stdlib.h>

#define ObjectFolderDefault ".service.fabric"
#define ObjectFolderDefaultW L".service.fabric"

#include "FabricSignal.h"

#endif
