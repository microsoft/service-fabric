// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"
#include "../../prod/src/data/tstore/stdafx.h"

#include <SfStatus.h>

namespace TStoreNightWatchTXRService
{
    using ::_delete;
}

#include "../NightWatchTXRService/NightWatchTXRService.Public.h"

#include "TestComStateProvider2Factory.h"
#include "TStoreTestRunner.h"
#include "TStoreService.h"
