// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define GET_RC( ResourceName ) \
    Common::StringResource::Get( IDS_HOSTING_##ResourceName )

#include "ktllogger/ktlloggerX.h"
#include "Common/Common.h"
#include "Common/KtlSF.Common.h"
#include "ServiceModel/ServiceModel.h"
#include "query/Query.h"
#include "client/ClientPointers.h"

#include "Hosting2/Hosting.h"
#include "Hosting2/Hosting.Runtime.h"
#include "Hosting2/IHostingSubsystem.h"
#include "Hosting2/ILegacyHostingHandler.h"

#include "Hosting2/Hosting.Internal.h"
#include "Hosting2/Hosting.Runtime.Internal.h"

#include "Hosting2/Hosting.FabricHost.h"

#include "Hosting2/Hosting.Protocol.h"
#include "Hosting2/Hosting.Protocol.Internal.h"

#include "Hosting.GuestApplicationRuntime.Internal.h"

#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h" 

#include <fstream>

#include "Hosting2/LeaseReply.h"

#if !defined(PLATFORM_UNIX)
#include <VersionHelpers.h>
#else
#include <libcgroup.h>
#include "Common/CryptoUtility.Linux.h"
#endif
