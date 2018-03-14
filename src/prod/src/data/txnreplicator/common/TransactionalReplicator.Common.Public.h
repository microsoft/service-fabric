// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#include <ktl.h>
#include <KTpl.h> 

namespace TxnReplicator
{
    using ::_delete;

    // Used in ITransactionManager.h
    class Transaction;
    // Used in ITransactionManager.h
    class AtomicOperation;
    // Used in ITransactionManager.h
    class AtomicRedoOperation;

    // Used in IStateProvider2.h
    interface ITransactionalReplicator;
}

namespace LoggingReplicatorTests
{
    using ::_delete;

    // Used in Transaction.h to define a friend
    class TestTransaction;
}

#include "Pointers.h"

#include "ITransaction.h"
#include "IRuntimeFolders.h"

#include "ApplyContext.h"
#include "OperationContext.h"
#include "LockContext.h"

#include "EnumerationCompletionResult.h"
#include "TryRemoveVersionResult.h"
#include "IVersionProvider.h"
#include "IVersionManager.h"
#include "ITransactionVersionManager.h"
#include "ITransactionManager.h"

#include "IOperationDataStream.h"
#include "OperationDataStream.h"
#include "TransactionState.h"
#include "TransactionBase.h"

#include "IDataLossHandler.h"

// Backup
#include "BackupVersion.h"
#include "BackupInfo.h"
#include "IBackupCallbackHandler.h"

// State Provider & notification related classes and interfaces.
#include "StateProviderInfo.h"
#include "IStateProvider2.h"
#include "ITransactionChangeHandler.h"
#include "IStateManagerChangeHandler.h"
#include "IStateProviderMap.h"
#include "ITransactionalReplicator.h"

#include "Transaction.h"
#include "AtomicOperation.h"
#include "AtomicRedoOperation.h"

#include "ServiceModel/ServiceModel.h"
#include "TransactionalReplicatorConfig.h"

#include "TRPerformanceCounters.h"
