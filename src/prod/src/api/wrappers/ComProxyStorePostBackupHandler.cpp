// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Api
{
    class ComProxyStorePostBackupHandler::PostBackupAsyncOperation
        : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(PostBackupAsyncOperation);

    public:
        PostBackupAsyncOperation(
            __in IFabricStorePostBackupHandler & comImpl,
            __in FABRIC_STORE_BACKUP_INFO const & info,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , comImpl_(comImpl)
            , info_(info)
            , status_(false)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, bool & status)
        {
            PostBackupAsyncOperation * casted = AsyncOperation::End<PostBackupAsyncOperation>(operation);

            if (casted->Error.IsSuccess())
            {
                status = std::move(casted->status_);
            }

            return casted->Error;
        }

        virtual ~PostBackupAsyncOperation()
        {
        }

    protected:

        HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            HRESULT hr = comImpl_.BeginPostBackup(&info_, callback, context);
            return hr;
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {						
            BOOLEAN userStatus = FALSE;
            HRESULT hr = comImpl_.EndPostBackup(context, &userStatus);

            status_ = SUCCEEDED(hr)
                ? userStatus ? true : false
                : false;
            
            return hr;
        }

    private:
        IFabricStorePostBackupHandler & comImpl_;
        FABRIC_STORE_BACKUP_INFO info_;
        bool status_;
    };


    ComProxyStorePostBackupHandler::ComProxyStorePostBackupHandler(Common::ComPointer<IFabricStorePostBackupHandler > const & comImpl)
        : ComponentRoot()
        , IStorePostBackupHandler()
        , comImpl_(comImpl)
    {
    }

    ComProxyStorePostBackupHandler::~ComProxyStorePostBackupHandler()
    {
    }

    Common::AsyncOperationSPtr ComProxyStorePostBackupHandler::BeginPostBackup(
        __in FABRIC_STORE_BACKUP_INFO const & info,
        __in Common::AsyncCallback const & callback,
        __in Common::AsyncOperationSPtr const & parent)
    {
        // this should invoke the customer's COM method BeginPostBackup
        // But how do we wrap the AsyncCallback and AsyncOperationSPtr into IFabricAsyncOperationCallback * and
        // IFabricAsyncOperationContext **
        // For this, we invoke a class derived from ComProxyAsyncOperation. This class contains two pure virtual
        // methods BeginComAsyncOperation and EndComAsyncOperation which do the actual conversion.

        auto operation = Common::AsyncOperation::CreateAndStart<PostBackupAsyncOperation>(			
            *comImpl_.GetRawPointer(),
            info,
            callback,
            parent);

        return operation;
    }

    Common::ErrorCode ComProxyStorePostBackupHandler::EndPostBackup(
        __in Common::AsyncOperationSPtr const & operation,
        __out bool & status)
    {
        auto error = PostBackupAsyncOperation::End(operation, status);
        return error;
    }
}
