// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Api
{
    // ********************************************************************************************************************
    // ComProxyStoreEventHandler::OnDataLossAsyncOperation  Implementation
    //
    class ComProxyStoreEventHandler::OnDataLossAsyncOperation
        : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(OnDataLossAsyncOperation);

    public:
        OnDataLossAsyncOperation(
            __in IFabricStoreEventHandler2 & comImpl,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , comImpl_(comImpl)
            , status_(false)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, bool & status)
        {
            OnDataLossAsyncOperation* onDataLossAsyncOperationPtr = AsyncOperation::End<OnDataLossAsyncOperation>(operation);

            if (onDataLossAsyncOperationPtr->Error.IsSuccess())
            {
                status = onDataLossAsyncOperationPtr->status_;
            }

            return onDataLossAsyncOperationPtr->Error;
        }

        virtual ~OnDataLossAsyncOperation()
        {
        }

    protected:

        HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            HRESULT hr = comImpl_.BeginOnDataLoss(callback, context);
            return hr;
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            BOOLEAN userStatus = FALSE;
            HRESULT hr = comImpl_.EndOnDataLoss(context, &userStatus);

            if (SUCCEEDED(hr))
            {
                // Setting explicitly to avoid C4800.
                status_ = userStatus ? true : false;
            }

            return hr;
        }

    private:
        IFabricStoreEventHandler2 & comImpl_;
        bool status_;
    };

    // ********************************************************************************************************************
    // ComProxyStoreEventHandler  Implementation
    //
    ComProxyStoreEventHandler::ComProxyStoreEventHandler(
        ComPointer<IFabricStoreEventHandler> const & comImpl)
        : ComponentRoot(),
        IStoreEventHandler(),
        comImpl_(comImpl),
        comImpl2_()
    {
    }

    ComProxyStoreEventHandler::ComProxyStoreEventHandler(
        ComPointer<IFabricStoreEventHandler2> const & comImpl2)
        : ComponentRoot(),
        IStoreEventHandler(),
        comImpl_(),
        comImpl2_(comImpl2)
    {
    }

    ComProxyStoreEventHandler::~ComProxyStoreEventHandler()
    {
    }

    void ComProxyStoreEventHandler::OnDataLoss(void)
    {
        ASSERT_IFNOT((comImpl_.GetRawPointer() == NULL) != (comImpl2_.GetRawPointer() == NULL), "Only one must be implemented.");

        if(comImpl_.GetRawPointer() != NULL)
        {
            comImpl_->OnDataLoss();
        }
        else
        {
            comImpl2_->OnDataLoss();
        }

        return;
    }

    Common::AsyncOperationSPtr ComProxyStoreEventHandler::BeginOnDataLoss(
        __in Common::AsyncCallback const & callback,
        __in Common::AsyncOperationSPtr const & state)
    {
        ASSERT_IFNOT((comImpl_.GetRawPointer() == NULL) != (comImpl2_.GetRawPointer() == NULL), "Only one must be implemented.");

        if (comImpl_.GetRawPointer() != NULL)
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCode(ErrorCodeValue::Success), callback, state);
        }
        else
        {
            auto operation = Common::AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(
                *comImpl2_.GetRawPointer(),
                callback,
                state);

            return operation;
        }
    }

    Common::ErrorCode ComProxyStoreEventHandler::EndOnDataLoss(
        __in Common::AsyncOperationSPtr const & operation,
        __out bool & status)
    {
        ASSERT_IFNOT((comImpl_.GetRawPointer() == NULL) != (comImpl2_.GetRawPointer() == NULL), "Only one must be implemented.");

        if (comImpl_.GetRawPointer() != NULL)
        {
            ASSERT_IF(dynamic_cast<CompletedAsyncOperation*>(operation.get()) == NULL, "Must have been a CompletedAsyncOperation.");

            return CompletedAsyncOperation::End(operation);
        }
        else
        {
            auto error = OnDataLossAsyncOperation::End(operation, status);
            return error;
        }
    }
}
