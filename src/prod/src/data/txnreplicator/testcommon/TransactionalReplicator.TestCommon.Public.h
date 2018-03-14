// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        using ::_delete;
    }
}

#include "VersionedState.h"

#include "TestAsyncOnDataLossContext.h"
#include "TestBackupAsyncOperation.h"
#include "TestRestoreAsyncOperation.h"
#include "TestBackupCallbackHandler.h"
#include "TestOperationDataStream.h"
#include "TestStateProviderProperties.h"
#include "TestLockContext.h"
#include "TestStateProvider.h"
#include "TestStateProvider.UndoOperationData.h"
#include "TestStateProviderFactory.h"

#include "NoopStateProvider.h"
#include "NoopStateProviderFactory.h"

#include "TestDataLossHandler.h"
#include "TestComProxyDataLossHandler.h"
#include "TestComStateProviderFactory.h"
