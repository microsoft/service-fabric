// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


#include "Reliability/Replication/reconfig.h"
#include "client/HealthReportingTransport.h"
#include "Management/ApplicationGateway/Http/HttpApplicationGatewayConfig.h"
#include "FabricClient.h"
#include "FabricClient_.h"

#include "systemservices/SystemServices.Fabric.h"
#include "Management/FileStoreService/IFileStoreClient.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

#include "Naming/NamingCommonEventSource.h"

#include "Naming/NamingServiceCuidCollection.h"
#include "Naming/NamingUtility.h"

#include "Naming/StaleRequestFailureReplyBody.h"
#include "Naming/namingmessages.h"

#include "Naming/PrimaryRecoveryHeader.h"

#include "Naming/INamingMessageProcessor.h"
#include "Naming/IGateway.h"
#include "Naming/IGatewayManager.h"

#include "Naming/NamingPointers.h"

#include "Naming/GatewayRetryHeader.h"
#include "Naming/Constants.h"
#include "Naming/NamingConfig.h"
#include "Naming/ClientAccessConfig.h"
#include "Naming/NamingService.h"
#include "Naming/namepropertykey.h"
#include "Naming/RequestInstance.h"
#include "Naming/requestinstancetracker.h"

#include "Naming/ActivityTracedComponent.h"

#include "Naming/BroadcastRequestEntry.h"

#include "Naming/BroadcastRequestVector.h"
#include "Naming/GatewayEventSource.h"
#include "Naming/PsdCacheEntry.h"
#include "Naming/PsdCache.h"
#include "Naming/BroadcastWaiters.h"
#include "Naming/BroadcastEventManager.h"
#include "Naming/ClientIdentityHeader.h"
#include "Naming/ServiceNotificationSender.h"
#include "Naming/ClientRegistrationTable.h"
#include "Naming/ServiceNotificationManager.h"
#include "Naming/FabricGatewayManager.h"
#include "Naming/FabricApplicationGatewayManager.h"
#include "Naming/GatewayManagerWrapper.h"
#include "Naming/GatewayProperties.h"
#include "Naming/GatewayNotificationReplyBuilder.h"

#include "Naming/FileTransferGateway.h"

#include "Naming/GatewayCounters.h"
#include "Naming/EntreeService.h"
#include "Naming/EntreeServiceIpcChannelBase.h"
#include "Naming/ProxyToEntreeServiceIpcChannel.h"
#include "Naming/ServiceNotificationManagerProxy.h"
#include "Naming/EntreeServiceProxy.h"
