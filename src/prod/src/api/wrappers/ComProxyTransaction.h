// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyTransaction :
        public ITransaction,
        public ComProxyTransactionBase
    {
        DENY_COPY(ComProxyTransaction);

    public:
        ComProxyTransaction(Common::ComPointer<IFabricTransaction> const & comImpl);
        virtual ~ComProxyTransaction();
        
        //
        // ITransactionBase methods
        // 
        Common::Guid get_Id();

        FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel();

        //
        // ITransaction methods
        // 
        Common::AsyncOperationSPtr BeginCommit(
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndCommit(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber);

        void Rollback();

    private:
        class CommitAsyncOperation;

    private:
        Common::ComPointer<IFabricTransaction> const comImpl_;
    };
}
