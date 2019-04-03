// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DynamicLonghaul
{
    class Service 
        : public TestableService::TestableServiceBase
    {
    public:
        Service(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

    protected:
        // Return a StateProviderFactory to the caller. This method will be called by the StatefulServiceBase.
        virtual Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() override;

        // Return a DataLossHandler to the caller. This method will be called by the StatefulServiceBase.
        Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler() override;

    private:
        // Enum used to differential test cases and create different state provider name.
        enum TestCase
        {
            Serially = 0,
            ConcurrentSameTask = 1,
            ConcurrentDifferentTask = 2
        };

        // Test used AddAsync or add path of GetOrAddAsync to add a state provider.
        enum AddMode
        {
            AddAsync = 0,
            GetOrAddAsync = 1,
        };

        // Test used Get or get path of GetOrAdd to check whether state provider exists.
        enum GetMode
        {
            GetOrAdd = 0,
            Get = 1,
        };

        enum TxnAction
        {
            Commit = 0,
            Abort = 1,
        };

    private:
        // Schedule work on client request.
        ktl::Awaitable<void> DoWorkAsync() override;

        // Get the result of DoWorkAsync.
        ktl::Awaitable<TestableService::TestableServiceProgress> GetProgressAsync() override;

        // Test cases
        ktl::Awaitable<void> VerifySerialCreationDeletionAsync(
            __in AddMode addMode,
            __in GetMode getMode);

        ktl::Awaitable<void> VerifyConcurrentCreationDeletionAsync(
            __in AddMode addMode,
            __in GetMode getMode);

        ktl::Awaitable<void> VerifyConcurrentCreationDeletionAndGetAsync();

        ktl::Awaitable<void> SafeTerminateReplicatorTransactionAsync(
            __in TxnReplicator::Transaction & replicatorTransaction,
            __in TxnAction txnAction);

        ktl::Awaitable<void> WorkLoadTask(
            __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
            __in ULONG taskID,
            __in ULONG numOfStateProvider,
            __in AddMode addMode,
            __in GetMode getMode);

        ktl::Awaitable<void> GetOrAddTask(
            __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount);

        ktl::Awaitable<void> DeletingTask(
            __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount);

        ktl::Awaitable<void> GetTask(
            __in ktl::AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount);

    private:// Helper functions
        KAllocator& GetAllocator();

        __declspec(noinline)
        KUri::CSPtr GetStateProviderName(
            __in TestCase testCase, 
            __in ULONG id,
            __in ULONG taskId = 0);

        bool IsPrefix(
            __in KUriView const & name,
            __in KStringView & prefix);

        void VerifyEnumeration(
            __in TestCase testCase,
            __in ULONG expectedCount);

    private:
        Common::Guid tracePartitionId_;
        FABRIC_PARTITION_ID partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        bool doWorkInProgress_;
    };
}