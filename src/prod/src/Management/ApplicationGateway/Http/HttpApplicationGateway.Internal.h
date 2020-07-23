// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "client/ResolvedServicePartitionResult.h"

#include "Constants.h"

#include "Utility.h"
#include "ServiceEndpointsList.h"
#include "GatewayRequestHandler.h"
#include "ServiceResolver.h"
#include "DummyServiceResolver.h"
#include "ParsedGatewayUri.h"
#include "GatewayRequestHandler.ProcessRequestAsyncOperation.h"
#include "GatewayRequestHandler.SendRequestToServiceAsyncOperation.h"
#include "GatewayRequestHandler.SendResponseToClientAsyncOperation.h"
#include "WebSocketIoForwarder.h"
#include "WebSocketManager.h"
#include "Common/KtlSF.Common.h"
