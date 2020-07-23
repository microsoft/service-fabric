// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace DynamicLonghaul;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;
using namespace Data;
using namespace TestableService;

static const StringLiteral TraceComponent("DynamicLonghaulService");

// Schedule work on client request.
// We take random step on different test case.
// 1. VerifySerialCreationDeletionAsync
// 2. VerifyConcurrentCreationDeletionAsync
ktl::Awaitable<void> Service::DoWorkAsync()
{
    // Protect against double DoWorkAsync call.
    // If doWork is in progress, just return. Otherwise set the flag to true.
    if (doWorkInProgress_)
    {
        co_return;
    }
    else
    {
        doWorkInProgress_ = true;
    }

    Common::Random random = Common::Random();
    int seed = random.Next() % 5;
    Trace.WriteInfo(
        TraceComponent, 
        "{0}:{1}: DoWorkAsync: Seed is {2}.", 
        tracePartitionId_, 
        replicaId_, 
        seed);

    try
    {
        switch (seed)
        {
        case 0:
        {
            co_await VerifySerialCreationDeletionAsync(AddMode::AddAsync, GetMode::Get);
            break;
        }
        case 1:
        {
            co_await VerifySerialCreationDeletionAsync(AddMode::GetOrAddAsync, GetMode::Get);
            break;
        }
        case 2:
        {
            co_await VerifyConcurrentCreationDeletionAsync(AddMode::AddAsync, GetMode::Get);
            break;
        }
        case 3:
        {
            co_await VerifyConcurrentCreationDeletionAsync(AddMode::GetOrAddAsync, GetMode::Get);
            break;
        }
        case 4:
        {
            co_await VerifyConcurrentCreationDeletionAndGetAsync();
            break;
        }
        default:
        {
            // Should not get here.
            CODING_ERROR_ASSERT(false);
        }
        }
    }
    catch (ktl::Exception & exception)
    {
        switch (exception.GetStatus())
        {
        case SF_STATUS_NOT_PRIMARY:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_OBJECT_CLOSED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_OBJECT_DISPOSED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case SF_STATUS_INVALID_OPERATION:
        {
            // TODO: Cover this case since state provider not registered throw invalid operation exception.
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        case STATUS_CANCELLED:
        {
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw exception;
        }
        default:
        {
            // TODO: need to think through all the exceptions it might throw
            Trace.WriteError(TraceComponent, "{0}:DoWorkAsync: Status: {1}.", replicaId_, exception.GetStatus());
            throw;
        }
        }
    }

    Trace.WriteInfo(TraceComponent, "{0}:DoWorkAsync: Completed.", replicaId_);
    doWorkInProgress_ = false;

    co_return;
}

// Get progress, it gets the state providers count on the replica 
ktl::Awaitable<TestableServiceProgress> Service::GetProgressAsync()
{
    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: Started.", replicaId_);
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // Verification
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);

    ULONG count = 0;

    while (enumerator->MoveNext())
    {
        ++count;
    }

    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: The state provider count is {1}", replicaId_, count);

    // Get the number of state providers in each store.
    TestableServiceProgress progress;
    progress.CustomProgress.push_back(count);

    status = TxReplicator->GetLastCommittedSequenceNumber(progress.LastCommittedSequenceNumber);
    THROW_ON_FAILURE(status);

    FABRIC_EPOCH e;
    status = TxReplicator->GetCurrentEpoch(e);
    THROW_ON_FAILURE(status);

    progress.DataLossNumber = e.DataLossNumber;
    progress.ConfigurationNumber = e.ConfigurationNumber;

    Trace.WriteInfo(TraceComponent, "{0}:GetProgressAsync: Completed.", replicaId_);

    co_return progress;
}

// Serial Creation and Deletion test.
// 1. Delete all the state providers added by this test case, with certain prefix.
// 2. Add state providers to the StateManager.
// 3. Verify all the state providers are added as expected.
// 4. Verify enumeration.
ktl::Awaitable<void> Service::VerifySerialCreationDeletionAsync(
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    // The prefix used to checkout the existence of state providers.
    KStringView prefix = L"fabric:/serial";
    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());
    ULONG numOfStateProviders = 30;
    IStateProvider2::SPtr stateProvider;

    // Populate state providers name list
    KArray<KUri::CSPtr> nameList(GetAllocator(), numOfStateProviders);
    for (ULONG index = 0; index < numOfStateProviders; ++index)
    {
        KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::Serially, index);
        status = nameList.Append(nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:{1}:VerifySerialCreationDeletionAsync: Append name to list must be successful.", 
            tracePartitionId_,
            replicaId_);
    }

    // Delete state providers if any exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));

            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    // Add state providers
    for (KUri::CSPtr nameSPtr : nameList)
    {
        Transaction::SPtr txnSPtr;
        THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
        KFinally([&] { txnSPtr->Dispose(); });

        if (addMode == AddMode::AddAsync)
        {
            status = co_await TxReplicator->AddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName);
            THROW_ON_FAILURE(status);
        }
        else
        {
            ASSERT_IFNOT(
                addMode == AddMode::GetOrAddAsync, 
                "{0}:VerifySerialCreationDeletionAsync: AddMode must be GetOrAddAsync.",
                replicaId_);
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName,
                stateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist == false,
                "{0}:{1}:VerifySerialCreationDeletionAsync: Add a state provider.",
                tracePartitionId_,
                replicaId_);
        }

        co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
    }

    // Verify all the state providers are added
    for (KUri::CSPtr nameSPtr : nameList)
    {
        if (getMode == GetMode::Get)
        {
            status = TxReplicator->Get(*nameSPtr, stateProvider);
            if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                throw ktl::Exception(status);
            }

            ASSERT_IFNOT(
                NT_SUCCESS(status), 
                "{0}:{1}: VerifySerialCreationDeletionAsync: State provider must be added.",
                tracePartitionId_,
                replicaId_);
        }
        else
        {
            ASSERT_IFNOT(
                getMode == GetMode::GetOrAdd, 
                "{0}:{1}:VerifySerialCreationDeletionAsync: GetMode must be GetOrAddAsync.",
                tracePartitionId_,
                replicaId_);
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName,
                stateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist,
                "{0}:{1}:VerifySerialCreationDeletionAsync: Get a state provider.",
                tracePartitionId_,
                replicaId_);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    VerifyEnumeration(TestCase::Serially, numOfStateProviders);

    co_return;
}

// Concurrent Creation and Deletion test.
// 1. Delete all the state providers added by this test case, with certain prefix.
// 2. Concurrently add state providers to the StateManager.
// 3. Verify all the state providers are added as expected.
// 4. Verify the enumeration.
ktl::Awaitable<void> Service::VerifyConcurrentCreationDeletionAsync(
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG concurrentExecutions = 20;
    ULONG workLoadPerTask = 25;

    KArray<Awaitable<void>> taskArray(GetAllocator());
    ASSERT_IFNOT(
        NT_SUCCESS(taskArray.Status()),
        "{0}:{1}:VerifyConcurrentCreationDeletionAsync: Failed to create TaskArray.",
        tracePartitionId_,
        replicaId_);
    
    AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
    status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
    ASSERT_IFNOT(
        NT_SUCCESS(status),
        "{0}:{1}:VerifyConcurrentCreationDeletionAsync: Failed to create the AwaitableCompletionSource.",
        tracePartitionId_,
        replicaId_);

    for (ULONG i = 0; i < concurrentExecutions; ++i)
    {
        status = taskArray.Append(WorkLoadTask(*signalCompletion, i, workLoadPerTask, addMode, getMode));
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:VerifyConcurrentCreationDeletionAsync: Failed to append WorkLoadTask to taskArray.",
            tracePartitionId_,
            replicaId_);
    }

    signalCompletion->SetResult(true);

    co_await (Utilities::TaskUtilities<void>::WhenAll(taskArray));

    VerifyEnumeration(TestCase::ConcurrentSameTask, concurrentExecutions * workLoadPerTask);

    co_return;
}

ktl::Awaitable<void> Service::WorkLoadTask(
    __in AwaitableCompletionSource<bool> & signalCompletion,
    __in ULONG taskID,
    __in ULONG numOfStateProvider,
    __in AddMode addMode,
    __in GetMode getMode)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IStateProvider2::SPtr outStateProvider = nullptr;

    wstring prefixString = wformatString("fabric:/concurrent/{0}", taskID);
    Trace.WriteInfo(TraceComponent, "{0}:VerifyConcurrentCreationDeletionAsync: prefixString: {1}.", replicaId_, prefixString);

    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());
    KStringView prefix(&prefixString[0]);

    KArray<KUri::CSPtr> nameList(GetAllocator(), numOfStateProvider);

    // Continue the current coroutine execution after being dispatched via a supplied KThreadPool
    // Invoke the test concurrency 
    co_await ktl::CorHelper::ThreadPoolThread(this->KtlSystemValue->DefaultThreadPool());

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    // Populate state providers name list
    for (ULONG index = 0; index < numOfStateProvider; ++index)
    {
        KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::ConcurrentSameTask, taskID * numOfStateProvider + index, taskID);
        status = nameList.Append(nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:WorkLoadTask: Append name to list must be successful.",
            tracePartitionId_,
            replicaId_);
    }

    // Delete state providers if any exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr = nullptr;
            status = TxReplicator->CreateTransaction(txnSPtr);
            THROW_ON_FAILURE(status);
            KFinally([txnSPtr] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    // Add state providers
    for (KUri::CSPtr nameSPtr : nameList)
    {
        Transaction::SPtr txnSPtr = nullptr;
        status = TxReplicator->CreateTransaction(txnSPtr);
        THROW_ON_FAILURE(status);
        KFinally([txnSPtr] { txnSPtr->Dispose(); });

        if (addMode == AddMode::AddAsync)
        {
            status = co_await TxReplicator->AddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName);
            THROW_ON_FAILURE(status);
        }
        else
        {
            ASSERT_IFNOT(
                addMode == AddMode::GetOrAddAsync, 
                "{0}:{1}:WorkLoadTask: AddMode must be GetOrAddAsync.",
                tracePartitionId_,
                replicaId_);
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist == false,
                "{0}:{1}:WorkLoadTask: Add a state provider.",
                tracePartitionId_,
                replicaId_);
        }

        co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
    }

    // Verify all the state providers are added
    for (KUri::CSPtr nameSPtr : nameList)
    {
        if (getMode == GetMode::Get)
        {
            status = TxReplicator->Get(*nameSPtr, outStateProvider);
            if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                throw ktl::Exception(status);
            }

            ASSERT_IFNOT(
                NT_SUCCESS(status), 
                "{0}:{1}:WorkLoadTask: State provider must be added.",
                tracePartitionId_,
                replicaId_);
        }
        else
        {
            ASSERT_IFNOT(
                getMode == GetMode::GetOrAdd, 
                "{0}:{1}:WorkLoadTask: GetMode must be GetOrAddAsync.",
                tracePartitionId_,
                replicaId_);
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                *nameSPtr,
                typeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            ASSERT_IFNOT(
                alreadyExist,
                "{0}:{1}:WorkLoadTask: Get a state provider.",
                tracePartitionId_,
                replicaId_);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

// Concurrently run 3 tasks, one task keeps on doing GetOrAdd, 
// one task keeps on deleting state provider and one task that keeps on getting state provider
// #9735493: Call AddOperation on state provider after we add or add the state provider.
ktl::Awaitable<void> Service::VerifyConcurrentCreationDeletionAndGetAsync()
{
    KStringView prefix = L"fabric:/concurrent/sp";
    ULONG taskCount = 30;

    KUri::CSPtr nameSPtr = GetStateProviderName(TestCase::ConcurrentDifferentTask, 0);

    KArray<Awaitable<void>> taskArray(GetAllocator());

    AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
    NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    status = taskArray.Append(GetOrAddTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    status = taskArray.Append(DeletingTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    status = taskArray.Append(GetTask(*signalCompletion, *nameSPtr, taskCount));
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    signalCompletion->SetResult(true);

    co_await (Utilities::TaskUtilities<void>::WhenAll(taskArray));

    Trace.WriteInfo(
        TraceComponent, 
        "{0}:VerifyConcurrentCreationDeletionAndGetAsync: All tasks completed, now delete the state provider if it still exists.", 
        replicaId_);

    // Delete the test state provider if it still exists.
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            Transaction::SPtr txnSPtr;
            THROW_ON_FAILURE(TxReplicator->CreateTransaction(txnSPtr));
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await TxReplicator->RemoveAsync(
                *txnSPtr,
                *currentKey);
            THROW_ON_FAILURE(status);

            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::GetOrAddTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IStateProvider2::SPtr outStateProvider = nullptr;
    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    co_await KTimer::StartTimerAsync(GetAllocator(), 'time', 100, nullptr);

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // GetOrAdd state provider with the name specified
        {
            Transaction::SPtr txnSPtr = nullptr;
            status = TxReplicator->CreateTransaction(txnSPtr);
            THROW_ON_FAILURE(status);
            KFinally([txnSPtr] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await TxReplicator->GetOrAddAsync(
                *txnSPtr,
                stateProviderName,
                typeName,
                outStateProvider,
                alreadyExist);
            THROW_ON_FAILURE(status);
            co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::DeletingTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    co_await KTimer::StartTimerAsync(GetAllocator(), 'time', 100, nullptr);

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // Delete the state provider if it exists.
        {
            Transaction::SPtr txnSPtr = nullptr;
            status = TxReplicator->CreateTransaction(txnSPtr);
            THROW_ON_FAILURE(status);
            KFinally([txnSPtr] { txnSPtr->Dispose(); });

            try
            {
                status = co_await TxReplicator->RemoveAsync(
                    *txnSPtr,
                    stateProviderName);
                THROW_ON_FAILURE(status);

                co_await SafeTerminateReplicatorTransactionAsync(*txnSPtr, TxnAction::Commit);
            }
            catch (Exception & exception)
            {
                if (exception.GetStatus() == SF_STATUS_NOT_PRIMARY)
                {
                    throw;
                }

                // The remove call may failed in the situation that two Remove calls happened in a row
                // Then Remove call will throw not exist exception.
                ASSERT_IFNOT(
                    exception.GetStatus() == SF_STATUS_NAME_DOES_NOT_EXIST,
                    "{0}:{1}:DeletingTask: RemoveAsync call throw exception {2}, the expected exception is {3}.",
                    tracePartitionId_,
                    replicaId_,
                    exception.GetStatus(),
                    SF_STATUS_NAME_DOES_NOT_EXIST);
            }
        }
    }

    co_return;
}

ktl::Awaitable<void> Service::GetTask(
    __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
    __in KUriView const & stateProviderName,
    __in ULONG taskCount)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    IStateProvider2::SPtr outStateProvider = nullptr;

    AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
    co_await tempCompletion->GetAwaitable();

    co_await KTimer::StartTimerAsync(GetAllocator(), 'time', 100, nullptr);

    for (ULONG i = 0; i < taskCount; ++i)
    {
        // The Get call may return STATUS_SUCCESS or SF_STATUS_NAME_DOES_NOT_EXIST based on Add/Remove sequence. So we don't care the result.
        // Need to get the result when we add AddOpertion call to state provider.
        status = TxReplicator->Get(stateProviderName, outStateProvider);
        if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            throw ktl::Exception(status);
        }
    }

    co_return;
}

// For different test case, we construct different state provider name
// Serially test name example: "fabric:/serial/sp/0", the number after sp stands 
//     for the StateProvider Id.
// Concurrently test name example: "fabric:/concurrent/0/sp/1", the first number
//     indicates the task id, the second number is the StateProvider Id.
KUri::CSPtr Service::GetStateProviderName(
    __in TestCase testCase, 
    __in ULONG id, 
    __in ULONG taskId)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KUri::CSPtr nameSPtr = nullptr;
    Trace.WriteInfo(TraceComponent, "{0}:GetStateProviderName: TestCase: {1}, Id: {2}, TaskId: {3}.", replicaId_, static_cast<int>(testCase), id, taskId);

    switch (testCase)
    {
    case TestCase::Serially:
    {
        wstring name = wformatString("fabric:/serial/sp/{0}", id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:{1}:GetStateProviderName: Failed to Create KUri in Serially case.",
            tracePartitionId_,
            replicaId_);
        break;
    }
    case TestCase::ConcurrentSameTask:
    {
        wstring name = wformatString("fabric:/concurrent/{0}/sp/{1}", taskId, id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:{1}:GetStateProviderName: Failed to Create KUri in ConcurrentSameTask case.",
            tracePartitionId_,
            replicaId_);
        break;
    }
    case TestCase::ConcurrentDifferentTask:
    {
        wstring name = wformatString("fabric:/concurrent/sp/{0}", id);
        status = KUri::Create(&name[0], GetAllocator(), nameSPtr);
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}:{1}:GetStateProviderName: Failed to Create KUri in ConcurrentDifferentTask case.",
            tracePartitionId_,
            replicaId_);
        break;
    }
    default:
    {
        ASSERT_IFNOT(
            false, 
            "{0}:{1}:GetStateProviderName: Should not reach here.",
            tracePartitionId_,
            replicaId_);
    }
    }

    Trace.WriteInfo(TraceComponent, "{0}:GetStateProviderName: KUir: {1}", replicaId_, ToStringLiteral(*nameSPtr));

    return nameSPtr;
}

ktl::Awaitable<void> Service::SafeTerminateReplicatorTransactionAsync(
    __in TxnReplicator::Transaction & replicatorTransaction,
    __in TxnAction txnAction)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    while (true)
    {
        bool doneCommitTx = false;
        try
        {
            if (txnAction == TxnAction::Commit)
            {
                THROW_ON_FAILURE(co_await replicatorTransaction.CommitAsync());
            }
            else
            {
                replicatorTransaction.Abort();
            }

            doneCommitTx = true;
        }
        catch (ktl::Exception & e)
        {
            Trace.WriteError(TraceComponent, "{0}:SafeTerminateReplicatorTransactionAsync: Status: {1}.", replicaId_, e.GetStatus());
            co_return;
        }

        if (doneCommitTx)
        {
            break;
        }

        status = co_await KTimer::StartTimerAsync(GetAllocator(), 'tsLD', 100, nullptr);
        ASSERT_IFNOT(
            NT_SUCCESS(status), 
            "{0}:{1}:SafeTerminateReplicatorTransactionAsync: Failed to start timer",
            tracePartitionId_,
            replicaId_);
    }

    co_return;
}

void Service::VerifyEnumeration(
    __in TestCase testCase,
    __in ULONG expectedCount)
{
    KStringView prefix = testCase == TestCase::Serially ? L"fabric:/serial" : L"fabric:/concurrent";

    ULONG count = 0;
    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
    NTSTATUS status = TxReplicator->CreateEnumerator(false, enumerator);
    THROW_ON_FAILURE(status);
    while (enumerator->MoveNext())
    {
        auto currentKey = enumerator->Current().Key;
        bool result = IsPrefix(*currentKey, prefix);
        if (result)
        {
            ++count;
        }
    }

    ASSERT_IFNOT(
        count == expectedCount,
        "{0}:{1}:VerifyEnumeration: Current count is {2}, and expected count is {3}.",
        tracePartitionId_,
        replicaId_,
        count,
        expectedCount);
}

bool Service::IsPrefix(
    __in KUriView const & name,
    __in KStringView & prefix)
{
    // TODO: KDynUri creation with const KUriView
    KDynUri candidate(static_cast<KUriView>(name), GetAllocator());
    KDynUri tempPrefix(prefix, GetAllocator());

    return tempPrefix.IsPrefixFor(candidate) == TRUE ? true : false;
}

KAllocator& Service::GetAllocator()
{
    return this->KtlSystemValue->PagedAllocator();
}

ComPointer<IFabricStateProvider2Factory> Service::GetStateProviderFactory()
{
    TxnReplicator::TestCommon::TestStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(GetAllocator());
    ComPointer<IFabricStateProvider2Factory> comFactory = TxnReplicator::TestCommon::TestComStateProvider2Factory::Create(
        *factory, GetAllocator());

    return comFactory;
}

ComPointer<IFabricDataLossHandler> Service::GetDataLossHandler()
{
    // False indicates no state change.
    return TxnReplicator::TestCommon::TestComProxyDataLossHandler::Create(GetAllocator());
}

Service::Service(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : TestableServiceBase(partitionId, replicaId, root)
    , tracePartitionId_(partitionId)
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , doWorkInProgress_(false)
{
}
