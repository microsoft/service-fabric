// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TestCommon
    {
        using ::_delete;
    }
    
    using ::_delete;
}

#include "../txnreplicator/common/TransactionalReplicator.Common.Public.h"

#include "ComAsyncOperationCallbackTestHelper.h"
#include "TestComStatefulServicePartition.h"
#include "TestComCodePackageActivationContext.h"
#include "TestStatefulServicePartition.h"
#include "TestRuntimeFolders.h"
#include "FileFormatTestHelper.h"

#define TEST_COMMIT_ASYNC(txnSPtr) \
    VERIFY_IS_TRUE(NT_SUCCESS(co_await txnSPtr->CommitAsync())) \

#define TEST_COMMIT_ASYNC_STATUS(txnSPtr, status) \
    VERIFY_ARE_EQUAL(co_await txnSPtr->CommitAsync(), status) \

