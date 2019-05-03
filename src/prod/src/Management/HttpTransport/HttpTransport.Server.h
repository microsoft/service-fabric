// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "FabricTypes.h"
#include "Transport/Transport.h"

#include "HttpTransport.Pointers.h"
#include "HttpUtil.h"
#include "Constants.h"

#include "LookasideAllocatorSettings.h"
#include "IRequestMessageContext.h"
#include "IHttpRequestHandler.h"
#include "IHttpServer.h"

#if !defined(PLATFORM_UNIX)
#include "IWebSocketHandler.h"
#include "WebSocketHandler.h"
#include "HttpServerWebSocket.h"
#endif
#include "HttpServer.h"
