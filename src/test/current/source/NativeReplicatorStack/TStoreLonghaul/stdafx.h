// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <atomic>
#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../TestableService/TestableService.Public.h"
#include "../../prod/src/data/utilities/Data.Utilities.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"
#include "../../prod/src/data/tstore/stdafx.h"

namespace TStoreLonghaul
{
    using ::_delete;
}

#include "TestComStateProvider2Factory.h"
#include "Service.h"
