// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CrashTestTXRService
{
    class Service
        : public TXRStatefulServiceBase::StatefulServiceBase
    {
    public:
        Service(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() override;

        Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler() override;

        Common::ErrorCode OnHttpPostRequest(
            __in Common::ByteBufferUPtr && body, 
            __in Common::ByteBufferUPtr & responseBody) override;

        void OnChangeRole(
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

    private:
        ktl::Task RunToCrash();
        ktl::Awaitable<void> AddSPAsync(__in std::wstring const & key, __in KAllocator & allocator);
        void GetRandomCrashChanceConfig();
        Common::Random random_;
        int64 randomCrashChance_;
    };
}
