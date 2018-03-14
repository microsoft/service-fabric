// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "HttpUtil.h"

#if !defined(PLATFORM_UNIX)
#include "ktlevents.um.h"
#endif

//
// Server side
//
#if !defined(PLATFORM_UNIX)
#include "RequestMessageContext.h"
#include "RequestMessageContext.ReadBodyAsyncOperation.h"
#include "RequestMessageContext.SendResponseAsyncOperation.h"
#include "HttpServer.AsyncKtlServiceOperationBase.h"
#include "HttpServer.OpenKtlAsyncServiceOperation.h"
#include "HttpServer.CloseKtlAsyncServiceOperation.h"

//
// Client side
//
#include "ClientRequest.h"
#include "ClientRequest.SendRequestAsyncOperation.h"
#include "ClientRequest.GetResponseBodyAsyncOperation.h"

#else
#include "requestmessagecontext.linux.h"
#include "RequestMessageContext.SendResponseAsyncOperation.Linux.h"
#include "requestmessagecontext.getfilefromuploadasyncoperation.linux.h"
#include "SimpleHttpServer.h"
#include "HttpServer.LinuxAsyncServiceBaseOperation.h"
#include "HttpServer.OpenLinuxAsyncServiceOperation.h"
#include "HttpServer.CloseLinuxAsyncServiceOperation.h"

#endif
