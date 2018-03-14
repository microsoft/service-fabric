// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RELIABLECOLLECTIONRUNTIME_TAG 'rcRT'
#define TEST_CEXPORT_TAG 'teCT'

// External dependencies
#include "../../txnreplicator/TransactionalReplicator.Public.h"
#include "../../tstore/LockMode.h"
#include "../../tstore/Store.Public.h"
#ifdef FEATURE_DICTIONARY_NOTIFICATION_BATCHING
#include "../../utilities/KAutoHashTable.h"
#include "../../tstore/Diagnostics.h"
#include "../../tstore/Dictionary.h"
#include "../../tstore/DictionaryEnumerator.h"
#include "../../tstore/ConcurrentDictionary2.h"
#endif
#include "../../utilities/StatusConverter.h"
#include <SfStatus.h>
#include "../../common/DataCommon.h"

namespace Data
{
    namespace Interop
    {
        using ::_delete;
    }
}

#ifndef WIN32
#define UNDER_PAL
#endif
#define _EXPORTING
#include "../ReliableCollectionRuntime/dll/ReliableCollectionRuntime.h"
#include "../ReliableCollectionRuntime/dll/ReliableCollectionRuntime.Internal.h"
#include "TestSettings.h"
#include "BackupCallbackHandler.h"
#include "DictionaryChangeHandler.h"
#include "StateManagerChangeHandler.h"
#include "TransactionChangeHandler.h"
#include "StateProviderInfo.h"
#include "StateProviderFactory.h"
#include "ComStateProviderFactory.h"

// External dependencies
// Test headers
#include "../../testcommon/TestCommon.Public.h"
#include "../../txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

// Helper macro's used by the tests which need a partitioned replica id trace type
#define TEST_TRACE_BEGIN(testName) \
        Trace.WriteWarning(TraceComponent, "{0} Begin Test Case: {1}", prId_->TraceId, testName); \
        KFinally([&]() { Trace.WriteWarning(TraceComponent, "{0} Finishing Test Case {1}", prId_->TraceId, testName); }); \

namespace Data
{
    namespace Integration
    {
        using ::_delete;
    }
}

#include "../../integration/TestBackupCallbackHandler.h"
#include "ComStateProviderFactory.h"
#include "Helper.h"

#include "../../integration/Replica.h"
