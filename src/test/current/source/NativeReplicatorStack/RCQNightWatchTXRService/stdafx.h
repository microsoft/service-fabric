#pragma once

#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"
#include "../../prod/src/data/tstore/stdafx.h"
#include "../../prod/src/data/reliableconcurrentqueue/stdafx.h"

#include <SfStatus.h>

namespace RCQNightWatchTXRService
{
    using ::_delete;
}

#include "../NightWatchTXRService/NightWatchTXRService.Public.h"

#include "TestComStateProvider2Factory.h"
#include "RCQTestRunner.h"
#include "RCQService.h"
