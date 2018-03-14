// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ITransaction : public ITransactionBase
    {
    public:
        virtual ~ITransaction() {};

        virtual Common::AsyncOperationSPtr BeginCommit(
            Common::TimeSpan const & timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndCommit(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber) = 0;

        virtual void Rollback() = 0;
    };
}
