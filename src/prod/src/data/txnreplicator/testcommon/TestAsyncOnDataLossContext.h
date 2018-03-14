// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestAsyncOnDataLossContext
            : public Ktl::Com::FabricAsyncContextBase
        {
            K_FORCE_SHARED(TestAsyncOnDataLossContext)

        public:
            static TestAsyncOnDataLossContext::SPtr Create(
                __in KAllocator & allocator,
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler);

        public:
            HRESULT StartOnDataLoss(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);

            HRESULT GetResult(__out BOOLEAN & isStateChanged);

        private:
            TestAsyncOnDataLossContext(
                __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler);

        protected:
            void OnStart();

        private:
            ktl::Task DoWorkAsync() noexcept;

        private:
            // Parent. Note that this can be pointing at nullptr.
            TxnReplicator::IDataLossHandler::SPtr dataLossHandlerSPtr_ = nullptr;

            // Result
            BOOLEAN isStateChanged_ = FALSE;
        };
    }
}
