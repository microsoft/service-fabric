// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    class TestableServiceBase
        : public ITestableService
        , public TXRStatefulServiceBase::StatefulServiceBase
    {
    public:
        TestableServiceBase(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        virtual ~TestableServiceBase();

    protected:
        //
        // HTTP Post request handler
        //
        virtual Common::ErrorCode OnHttpPostRequest(
            __in Common::ByteBufferUPtr && body,
            __in Common::ByteBufferUPtr & responseBody) override;

    private:
        //
        // This method sets the status of work to 'InProgress', and calls DoWork.
        // Once DoWork returns, it will set the status to 'Done'.
        //
        ktl::Task DoWorkAsyncWrapper();

    private:
        std::atomic<WorkStatusEnum::Enum> workStatus_;
        NTSTATUS statusCode_;
    };
}
