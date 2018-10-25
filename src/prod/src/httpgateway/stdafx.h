// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "HttpGateway.h"
#if !defined(PLATFORM_UNIX)
#include "HttpGatewayEventSource.h"
#endif
#include "HttpGateway.Internal.h"
#include <fstream>

#include "ContainerApiCall.h"
#include "Hosting2/HostingConfig.h"

using namespace std;
