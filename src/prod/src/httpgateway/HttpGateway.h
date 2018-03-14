// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "Common/Common.h"
#include "FabricClient.h"

#if !defined(PLATFORM_UNIX)
#include "khttp.h"
#endif

#include "Transport/Transport.h"
#include "Management/HttpTransport/HttpTransport.Server.h"
#if !defined(PLATFORM_UNIX)
#include "Management/HttpTransport/HttpTransport.Client.h"
#include "Management/ApplicationGateway/Http/HttpApplicationGateway.Pointers.h"
#include "Management/ApplicationGateway/Http/IHttpApplicationGateway.h"
#include "Management/ApplicationGateway/Http/IHttpApplicationGatewayConfig.h"
#include "Management/ApplicationGateway/Http/IHttpApplicationGatewayEventSource.h"
#endif

//
// HttpGateway Public Header Files
//
#if !defined(PLATFORM_UNIX)
#include "httpgateway/HttpGatewayEventSource.h"
#endif
#include "httpgateway/HttpGateway.Pointers.h"
#include "httpgateway/HttpGatewayConfig.h"
#include "httpgateway/HttpServer.h"
