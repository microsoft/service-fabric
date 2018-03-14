// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#define API_DISPATCHER_TAG 'DipA'
#define METADATA_TAG 'dmMS'
#define METADATA_MANAGER_TAG 'gmDM'
#define SM_TRANSACTIONCONTEXT_TAG 'clXT'
#define STATE_MANAGER_TAG 'rgMS'
#define REPLICATION_METADATA_TAG 'dmER'
#define NAMED_OPERATION_CONTEXT_TAG 'cpON'
#define NAMED_OPERATION_DATA_TAG 'dpON'
#define COPY_NAMED_OPERATION_DATA_TAG 'poNC'
#define METADATA_OPERATION_DATA_TAG 'dpOM'
#define REDO_OPERATION_DATA_TAG 'dpOR'
#define SERIALIZABLEMETADATA_TAG 'msMS'
#define CHECKPOINTFILE_TAG 'lfMS'
#define Checkpoint_FILE_ASYNC_ENUMERATOR_TAG 'eafC'
#define CHECKPOINTFILEBLOCKS_TAG 'bfMS'
#define CHECKPOINTFILEPROPERTIES_TAG 'pfMS'
#define CHECKPOINT_MANAGER_TAG 'rgMC'
#define NAMED_OPERATION_DATA_STREAM_TAG 'sdON'
#define OPERATION_DATA_ENUMERATOR 'mnDO'
#define SERIALIZABLE_METADATA_ENUMERATOR 'mnMS'
#define ACTIVES_STATEPROVIDER_ENUMERATOR 'mnSA'

#define TEST_TAG 'gaTT'
#define TEST_OPERATIONCONTEXT 'coTT'
#define TEST_LOCKCONTEXT 'clTT'
#define TEST_STATE_PROVIDER_FACTORY_TAG 'fPST'
#define TEST_STATE_PROVIDER_PROPERTY_TAG 'ppST'

#define NULL_OPERATIONDATA -1

#define TRACE_ERRORS(status, pId, rId, component, ...)\
{\
    Helper::TraceErrors(\
        status,\
        pId,\
        rId,\
        component,\
        Helper::StringFormat(__VA_ARGS__));\
}

// External dependencies
#if defined(PLATFORM_UNIX)
#include "Common/Common.h"
#endif
#include "ktl.h" // SFStatus.h requires
#include <SfStatus.h>
#include "../../data.public.h"
#include "../../utilities/Data.Utilities.Public.h"
#include "../common/TransactionalReplicator.Common.Public.h"
#include "../common/TransactionalReplicator.Common.Internal.h"

// Test external dependencies
#include "../../testcommon/TestCommon.Public.h"

// StateManager public
#include "StateManager.Public.h"

// Internal usage
#include "StateManager.Internal.h"
#include "TracingHeaders.h"

namespace Data
{
    namespace StateManager
    {
        // Lock Context requires
        class MetadataManager;
        class StateManagerTransactionContext;

        // Metadata requires
        class SerializableMetadata;
    }
}

// Enums
#include "StateManagerLockMode.h"
#include "OperationType.h"

// Internal
#include "StateProviderInfoInternal.h"
#include "StateProviderIdentifier.h"
#include "OpenParameters.h"
#include "Metadata.h"
#include "StateProviderFilterMode.h"
#include "ActiveStateProviderEnumerator.h"
#include "SerializableMetadata.h"
#include "NamedOperationContext.h"
#include "MetadataOperationData.h"
#include "RedoOperationData.h"
#include "NamedOperationData.h"
#include "CopyNamedOperationData.h"
#include "OperationDataEnumerator.h"
#include "CopyOperationDataStream.h"
#include "NamedOperationDataStream.h"
#include "StateManagerLockContext.h"
#include "StateManagerTransactionContext.h"
#include "StateManagerCopyOperation.h"
#include "Comparer.h"
#include "Helper.h"

// Checkpoint 
#include "CheckpointFileProperties.h"
#include "CheckpointFileBlocks.h"

namespace Data
{
    namespace StateManager
    {
        // CheckpointFile requires
        class SerializableMetadataEnumerator;
        class CheckpointFileAsyncEnumerator;
    }
}

#include "CheckpointFileAsyncEnumerator.h"
#include "CheckpointFile.h"
#include "SerializableMetadataEnumerator.h"

// Managers
#include "ApiDispatcher.h"
#include "MetadataManager.h"
#include "CheckpointManager.h"

namespace StateManagerTests
{
    using ::_delete;

    class RecoveryTest;
    class StateManagerTest;
}

#include "StateManager.h"

// The following are test headers
#include "FaultStateProviderType.h"
#include "TestOperationContext.h"
#include "TestOperation.h"
#include "TestLockContext.h"
#include "TestOperationDataStream.h"
#include "TestReplicatedOperation.h"
#include "TestTransaction.h"
#include "TestStateProviderFactory.h"
#include "TestStateProviderProperties.h"
#include "TestStateProvider.h"
#include "TestHelper.h"
#include "MockLoggingReplicator.h"
#include "TestTransactionManager.h"
#include "TestCheckpointFileReadWrite.h"
#include "StateManagerTestBase.h"
#include "TestStateManagerChangeHandler.h"
