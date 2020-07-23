// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <Common/Common.h>
#include <FabricRuntime.h>

#define ROBUSTTEST_TAG 'gttr'

#include "../../prod/src/data/txnreplicator/TransactionalReplicator.Public.h"

#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

namespace RobustTXRService
{
    using ::_delete;
}

#include "../NightWatchTXRService/NightWatchTXRService.Public.h"

#include "TestParameters.h"
#include "RobustTestRunner.h"
#include "RobustService.h"
