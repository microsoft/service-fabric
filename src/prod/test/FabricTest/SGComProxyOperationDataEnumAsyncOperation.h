// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace TestCommon;

namespace FabricTest
{
    class SGComProxyOperationDataEnumAsyncOperation : 
        public Common::ComProxyAsyncOperation
    {
        DENY_COPY(SGComProxyOperationDataEnumAsyncOperation);

    public:
        SGComProxyOperationDataEnumAsyncOperation(
            Common::ComPointer<IFabricOperationDataStream> const & enumerator,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationDataEnumAsyncOperation::SGComProxyOperationDataEnumAsyncOperation ({0}) - ctor",
                this);

            enumerator_ = enumerator;
        }

        virtual ~SGComProxyOperationDataEnumAsyncOperation()
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationDataEnumAsyncOperation::~SGComProxyOperationDataEnumAsyncOperation ({0}) - dtor",
                this);
        }

        virtual HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationDataEnumAsyncOperation::BeginComAsyncOperation ({0}) - BeginGetNext",
                this);

            return enumerator_->BeginGetNext(callback, context);
        }

        virtual HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyOperationDataEnumAsyncOperation::EndComAsyncOperation ({0}) - EndGetNext",
                this);

            return enumerator_->EndGetNext(context, operationData_.InitializationAddress());
        }

        __declspec (property(get=get_OperationData)) Common::ComPointer<IFabricOperationData> & OperationData;
        Common::ComPointer<IFabricOperationData> & get_OperationData() { return operationData_; }

    private:
        Common::ComPointer<IFabricOperationDataStream> enumerator_;

        Common::ComPointer<IFabricOperationData> operationData_;

        static Common::StringLiteral const TraceSource;
    };
};
