// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#include <ktl.h>
#include <KTpl.h> 

namespace Data
{
    namespace LoggingReplicator
    {
        using ::_delete;
    }
}

namespace LoggingReplicatorTests
{
    using ::_delete;

    // Used in TransactionMap.h to define a friend
    class TransactionMapTest;

    // Used in BackupManager.h to define a friend
    class BackupManagerTests;
}


#include "KComAdapter.h"

#include "IOperation.h"
#include "IOperationStream.h"
#include "IStateProvider.h"
#include "IStateReplicator.h"

#include "LoggingReplicatorFactory.h"
