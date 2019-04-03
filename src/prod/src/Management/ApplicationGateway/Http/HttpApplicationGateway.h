// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "client/client.h"
#include "ServiceModel/ServiceModel.h"
#include "FabricTypes.h"

#include "Management/HttpTransport/HttpTransport.Server.h"
#include "Management/HttpTransport/HttpTransport.Client.h"

#include "TargetReplicaSelector.h"
#include "IHttpApplicationGatewayConfig.h"
#include "HttpApplicationGatewayConfig.h"
#include "IHttpApplicationGatewayEventSource.h"
#include "HttpApplicationGatewayEventSource.h"
#include "HttpApplicationGateway.Pointers.h"
#include "IHttpApplicationGateway.h"
#include "IHttpApplicationGatewayHandlerOperation.h"
#include "IServiceResolver.h"
#include "HttpApplicationGatewayImpl.h"
