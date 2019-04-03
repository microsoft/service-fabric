// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <Common/Common.h>
#include <FabricRuntime.h>

// Base dependencies
#include "../../prod/src/data/txnreplicator/TransactionalReplicator.Public.h"

#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

namespace V1ReplPerf
{
    using ::_delete;
}

#include <SfStatus.h>

#include "../NightWatchTXRService/NightWatchTXRService.Public.h"

#include "ComAsyncOperationCallbackHelper.h"
#include "Constants.h"
#include "OperationData.h"
#include "ServiceEntry.h"
#include "Test.h"
#include "TestClientRequestHandler.h"
#include "Service.h"
#include "ComService.h"
#include "Factory.h"
#include "ComFactory.h"
