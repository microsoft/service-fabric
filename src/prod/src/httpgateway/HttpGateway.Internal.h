// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// External headers required by HttpGateway.
//
#if !defined (PLATFORM_UNIX)
#include <Shlwapi.h>
#include "ktlevents.um.h"
#endif

#include "AzureActiveDirectory/wrapper.server/ServerWrapper.h"
#include "query/Query.h"
#include "ServiceModel/ServiceModel.h"
#include "Reliability/Reliability.h"
#include "client/client.h"
#if !defined (PLATFORM_UNIX)
#include "client/ResolvedServicePartitionResult.h"
#endif
#include "Management/ImageStore/ImageStore.h"
#include "Management/FileStoreService/FileStoreService.h"
#include "Common/KtlSF.Common.h"

#if !defined (PLATFORM_UNIX)
#include "Management/ApplicationGateway/Http/HttpApplicationGateway.Common.h"
#endif

//
// Header Files requires by the internal HttpGateway components
//

#include "httpgateway/Constants.h"
#include "httpgateway/ErrorCodeMessageBody.h"
#include "httpgateway/ErrorBody.h"
#include "httpgateway/HandlerUriTemplate.h"
#include "httpgateway/GatewayUri.h"
#include "httpgateway/Utility.h"
#include "httpgateway/FabricClientWrapper.h"
#include "httpgateway/RequestHandlerBase.h"
#include "httpgateway/RequestHandlerBase.HandlerAsyncOperation.h"
#include "httpgateway/ApplicationUpgradeProgress.h"
#include "httpgateway/ApplicationsHandler.h"
#include "httpgateway/ApplicationTypesHandler.h"
#include "httpgateway/SecretsResourceHandler.h"
#include "httpgateway/ComposeDeploymentsHandler.h"
#include "httpgateway/VolumesHandler.h"
#include "httpgateway/NodesHandler.h"
#include "httpgateway/NamesHandler.h"
#include "httpgateway/ApplicationsResourceHandler.h"
#include "httpgateway/NetworksHandler.h"
#include "httpgateway/GatewaysResourceHandler.h"
#include "httpgateway/FabricUpgradeProgress.h"
#include "httpgateway/FabricOrchestrationUpgradeProgress.h"
#include "httpgateway/DeployedApplicationQueryResultWrapper.h"
#include "httpgateway/ClusterManagementHandler.h"
#include "httpgateway/HttpServer.CreateAndOpenAsyncOperation.h"
#include "httpgateway/HttpServer.OpenAsyncOperation.h"
#include "httpgateway/HttpServer.CloseAsyncOperation.h"
#include "httpgateway/HttpAuthHandler.h"
#include "httpgateway/imagestorehandler.h"
#include "httpgateway/InvokeDataLossProgress.h"
#include "httpgateway/InvokeQuorumLossProgress.h"
#include "httpgateway/RestartPartitionProgress.h"
#include "httpgateway/HttpCertificateAuthHandler.h"
#include "httpgateway/ToolsHandler.h"
#include "httpgateway/HttpClaimsAuthHandler.h"
#include "httpgateway/NodeTransitionProgress.h"

#if !defined (PLATFORM_UNIX)
#include "httpgateway/BackupRestoreHandler.h"
#include "httpgateway/getfilefromuploadasyncoperation.h"
#include "httpgateway/HttpKerberosAuthHandler.h"
#include "httpgateway/ServiceResolver.h"
#include "httpgateway/EventsStoreHandler.h"
#endif

#include "httpgateway/HttpServer.AccessCheckAsyncOperation.h"
#include "httpgateway/HttpAuthHandler.AccessCheckAsyncOperation.h"

#include "httpgateway/UriArgumentParser.h"
#include "httpgateway/FaultsHandler.h"
