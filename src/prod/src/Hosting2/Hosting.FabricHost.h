// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "Federation/Federation.h"
#include "Common/Common.h"
#include "Common/TraceSession.h"
#include "ServiceModel/ServiceModel.h"
#include "ServiceModel/management/ResourceMonitor/public.h"

//
// Hosting header files used by FabricHost (exe)
//
#include "Hosting2/HostedServiceParameters.h"
#include "Hosting2/HostingPointers.h"
#include "Hosting2/Constants.h"
#include "Hosting2/SecurityIdentityMap.h"
#include "Hosting2/FabricHostConfig.h"
#if !defined(PLATFORM_UNIX)
#include <netfw.h>
#include <wrl/client.h>
#include "Hosting2/FirewallSecurityProviderHelper.h"
#endif
#include "Hosting2/FabricActivationManager.h"
#include "Hosting2/FabricRestartManager.h"
#include "Hosting2/HostedServiceActivationManager.h"
#include "Hosting2/ProcessActivationManager.h"
#include "Hosting2/HostedService.h"
#include "Hosting2/HostedServiceMap.h"
#include "Hosting2/HostedServiceInstance.h"
#include "Hosting2/HostedServiceActivationManager.h"
#include "Hosting2/ApplicationService.h"
#include "Hosting2/ApplicationServiceMap.h"

