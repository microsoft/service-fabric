// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace TestCommon;

namespace FabricTest
{
    class SGComProxyOperationStreamAsyncOperation : 
        public Common::ComProxyAsyncOperation
    {
        DENY_COPY(SGComProxyOperationStreamAsyncOperation);

    public:
        SGComProxyOperationStreamAsyncOperation(
            Common::ComPointer<IFabricOperationStream> const & operationStream,
            bool isCopy,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , operationStream_(operationStream)
            , isCopy_(isCopy)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationStreamAsyncOperation::SGComProxyOperationStreamAsyncOperation ({0}) - ctor",
                this);
        }

        virtual ~SGComProxyOperationStreamAsyncOperation()
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationStreamAsyncOperation::~SGComProxyOperationStreamAsyncOperation ({0}) - dtor",
                this);
        }

        virtual HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationStreamAsyncOperation::BeginComAsyncOperation ({0}) - BeginGetOperation",
                this);

            return operationStream_->BeginGetOperation(callback, context);
        }

        virtual HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationStreamAsyncOperation::EndComAsyncOperation ({0}) - EndGetOperation",
                this);

            return operationStream_->EndGetOperation(context, operation_.InitializationAddress());
        }

        __declspec (property(get=get_Operation)) Common::ComPointer<IFabricOperation> & Operation;
        Common::ComPointer<IFabricOperation> & get_Operation() { return operation_; }

        __declspec (property(get=get_IsCopy)) bool IsCopy;
        bool get_IsCopy() { return isCopy_; }

    private:
        Common::ComPointer<IFabricOperationStream> const & operationStream_;

        Common::ComPointer<IFabricOperation> operation_;
        bool isCopy_;

        static Common::StringLiteral const TraceSource;
    };
};
