// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include <mutex>
#include "targetver.h"

#include "Common/Common.h"
#include "Common/ConfigEventSource.h"

#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Transport/Transport.h"
#include "Federation/Federation.h"

#include "ConfigLoader.h"

#if !defined(PLATFORM_UNIX)
#include "TlHelp32.h"
#include "Dbghelp.h"
#endif

