// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// ComProxyCodePackageHost::ActivateAsyncOperation Implementation
//
class ComProxyCodePackageHost::ActivateAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        ComProxyCodePackageHost & owner,
        wstring const & codePackageId,
        ComPointer<IFabricCodePackageActivationContext> const & activationContext,
        ComPointer<IFabricRuntime> const & fabricRuntime,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent),
        owner_(owner),
        codePackageId_(codePackageId),
        activationContext_(activationContext),
        fabricRuntime_(fabricRuntime)
    {
    }

    virtual ~ActivateAsyncOperation() 
    { 
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
     HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
     {
         return owner_.codePackageHost_->BeginActivate(
             codePackageId_.c_str(),
             activationContext_.GetRawPointer(),
             fabricRuntime_.GetRawPointer(),
             callback,
             context);
     }

     HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
     {
         return owner_.codePackageHost_->EndActivate(context);
     }

private:
    ComProxyCodePackageHost & owner_;
    wstring const codePackageId_;
    ComPointer<IFabricCodePackageActivationContext> const & activationContext_;
    ComPointer<IFabricRuntime> const & fabricRuntime_;
};


// ********************************************************************************************************************
// ComProxyCodePackageHost::DeactivateAsyncOperation Implementation
//
class ComProxyCodePackageHost::DeactivateAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        ComProxyCodePackageHost & owner,
        wstring const & codePackageId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent),
        owner_(owner),
        codePackageId_(codePackageId)
    {
    }

    virtual ~DeactivateAsyncOperation() 
    { 
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
     HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
     {
         return owner_.codePackageHost_->BeginDeactivate(
             codePackageId_.c_str(),
             callback,
             context);
     }

     HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
     {
         return owner_.codePackageHost_->EndDeactivate(context);
     }

private:
    ComProxyCodePackageHost & owner_;
    wstring const codePackageId_;
};

// ********************************************************************************************************************
// ComProxyCodePackageHost Implementation
//
ComProxyCodePackageHost::ComProxyCodePackageHost(ComPointer<IFabricCodePackageHost> const & codePackageHost)
    : codePackageHost_(codePackageHost)
{
}

ComProxyCodePackageHost::~ComProxyCodePackageHost()
{
}

AsyncOperationSPtr ComProxyCodePackageHost::BeginActivate(
    wstring const & codePackageId,
    ComPointer<IFabricCodePackageActivationContext> const & activationContext,
    ComPointer<IFabricRuntime> const & fabricRuntime,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        codePackageId,
        activationContext,
        fabricRuntime,
        callback,
        parent);
}

ErrorCode ComProxyCodePackageHost::EndActivate(AsyncOperationSPtr const & asyncOperation)
{
    return ActivateAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyCodePackageHost::BeginDeactivate(
    wstring const & codePackageId,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        codePackageId,
        callback,
        parent);
}

ErrorCode ComProxyCodePackageHost::EndDeactivate(AsyncOperationSPtr const & asyncOperation)
{
    return DeactivateAsyncOperation::End(asyncOperation);
}
