// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// External Header Files required by public Hosting header files
//
#include "ktllogger/KtlLoggerPointers.h"
#include "Federation/Federation.h"
#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Transport/Transport.h"
#include "query/Query.h"
#include "client/client.h"
#include "client/ClientPointers.h"
//
// Hosting Public Header Files
//
#include "Hosting2/HostingPointers.h"
#include "Hosting2/ILegacyHostingHandler.h"
#include "Hosting2/HostingConfig.h"
#include "Hosting2/RunAsConfig.h"
#include "Hosting2/HostingEvents.h"
#include "Hosting2/IFabricUpgradeImpl.h"
#include "Hosting2/IHostingSubsystem.h"
#include "Hosting2/HostedServiceParameters.h"
#include "Hosting2/ContainerApiResult.h"
