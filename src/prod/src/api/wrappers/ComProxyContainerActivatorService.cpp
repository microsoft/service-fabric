// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComProxyContainerActivatorService");

class ComProxyContainerActivatorService::ActivateContainerAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ActivateContainerAsyncOperation)

public:
    ActivateContainerAsyncOperation(
        IFabricContainerActivatorService & comImpl,
        ContainerActivationArgs const & activationArgs,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , activationArgs_(activationArgs)
        , timeout_(timeout)
    {
    }

    virtual ~ActivateContainerAsyncOperation() { }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out std::wstring & containerId)
    {
        auto thisPtr = AsyncOperation::End<ActivateContainerAsyncOperation>(operation);

        if (!(thisPtr->Error.IsSuccess()))
        {
            return thisPtr->Error;
        }
        
        auto fabricStr = thisPtr->result_->get_String();
        auto error = StringUtility::LpcwstrToWstring2(fabricStr, false, containerId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "StringUtility::LpcwstrToWstring2: Failed to parse containerId: Error={0}.",
                error);
        }
        
        return error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ScopedHeap heap;
        FABRIC_CONTAINER_ACTIVATION_ARGS fabricActivationArgs = {};

        auto error = activationArgs_.ToPublicApi(heap, fabricActivationArgs);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "activationArgs_.ToPublicApi() failed with Error={0}.",
                error);

            return error.ToHResult();
        }

        return comImpl_.BeginActivateContainer(
            &fabricActivationArgs,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndActivateContainer(context, &result_);
    }

private:
    IFabricContainerActivatorService & comImpl_;
    ContainerActivationArgs activationArgs_;
    TimeSpan timeout_;
    IFabricStringResult * result_;
};

class ComProxyContainerActivatorService::DeactivateContainerAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DeactivateContainerAsyncOperation)

public:
    DeactivateContainerAsyncOperation(
        IFabricContainerActivatorService & comImpl,
        ContainerDeactivationArgs const & deactivationArgs,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , deactivationArgs_(deactivationArgs)
        , timeout_(timeout)
    {
    }

    virtual ~DeactivateContainerAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ScopedHeap heap;
        FABRIC_CONTAINER_DEACTIVATION_ARGS fabricDeactivationArgs = {};

        auto error = deactivationArgs_.ToPublicApi(heap, fabricDeactivationArgs);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "deactivationArgs_.ToPublicApi() failed with Error={0}.",
                error);

            return error.ToHResult();
        }

        return comImpl_.BeginDeactivateContainer(
            &fabricDeactivationArgs,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndDeactivateContainer(context);
    }

private:
    IFabricContainerActivatorService & comImpl_;
    ContainerDeactivationArgs deactivationArgs_;
    TimeSpan timeout_;
};

class ComProxyContainerActivatorService::InvokeContainerApiAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(InvokeContainerApiAsyncOperation)

public:
    InvokeContainerApiAsyncOperation(
        IFabricContainerActivatorService & comImpl,
        ContainerApiExecutionArgs const & apiExecArgs,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , apiExecArgs_(apiExecArgs)
        , timeout_(timeout)
    {
    }

    virtual ~InvokeContainerApiAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, ContainerApiExecutionResponse & apiExecResp)
    {
        auto thisPtr = AsyncOperation::End<InvokeContainerApiAsyncOperation>(operation);

        if (!(thisPtr->Error.IsSuccess()))
        {
            return thisPtr->Error;
        }

        auto fabricResult = thisPtr->result->get_Result();
        auto error = apiExecResp.FromPublicApi(*fabricResult);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "apiExecResp.FromPublicApi() failed with Error={0}.",
                error);
        }

        return error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ScopedHeap heap;
        FABRIC_CONTAINER_API_EXECUTION_ARGS execArgs = {};

        auto error = apiExecArgs_.ToPublicApi(heap, execArgs);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "apiExecArgs_.ToPublicApi() failed with Error={0}.",
                error);

            return error.ToHResult();
        }

        return comImpl_.BeginInvokeContainerApi(
            &execArgs,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndInvokeContainerApi(context, &result);
    }

private:
    IFabricContainerActivatorService & comImpl_;
    ContainerApiExecutionArgs apiExecArgs_;
    TimeSpan timeout_;
    IFabricContainerApiExecutionResult * result;
};

class ComProxyContainerActivatorService::DownloadContainerImagesAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DownloadContainerImagesAsyncOperation)

public:
    DownloadContainerImagesAsyncOperation(
        IFabricContainerActivatorService & comImpl,
        vector<ContainerImageDescription> const& images,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , images_(images)
        , timeout_(timeout)
    {
    }

    virtual ~DownloadContainerImagesAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ScopedHeap heap;
        FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST fabricImageList = {};

        auto error = PublicApiHelper::ToPublicApiList<
            ContainerImageDescription, 
            FABRIC_CONTAINER_IMAGE_DESCRIPTION, 
            FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST>(heap, images_, fabricImageList);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "PublicApiHelper::ToPublicApiList() for contaier download failed with Error={0}.",
                error);

            return error.ToHResult();
        }

        return comImpl_.BeginDownloadImages(
            &fabricImageList,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndDownloadImages(context);
    }

private:
    IFabricContainerActivatorService & comImpl_;
    vector<ContainerImageDescription> images_;
    TimeSpan timeout_;
};

class ComProxyContainerActivatorService::DeleteContainerImagesAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DeleteContainerImagesAsyncOperation)

public:
    DeleteContainerImagesAsyncOperation(
        IFabricContainerActivatorService & comImpl,
        vector<wstring> const& images,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , images_(images)
        , timeout_(timeout)
    {
    }

    virtual ~DeleteContainerImagesAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ScopedHeap heap;
        auto fabricImageList = heap.AddItem<FABRIC_STRING_LIST>();

        StringList::ToPublicAPI(heap, images_, fabricImageList);

        return comImpl_.BeginDeleteImages(
            fabricImageList.GetRawPointer(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndDeleteImages(context);
    }

private:
    IFabricContainerActivatorService & comImpl_;
    vector<wstring> images_;
    TimeSpan timeout_;
};

ComProxyContainerActivatorService::ComProxyContainerActivatorService(
    ComPointer<IFabricContainerActivatorService> const & comImpl)
    : IContainerActivatorService()
    , comImpl_(comImpl)
{
}

ComProxyContainerActivatorService::~ComProxyContainerActivatorService()
{
}

ErrorCode ComProxyContainerActivatorService::StartEventMonitoring(
    bool isContainerServiceManaged,
    uint64 sinceTime)
{
    auto hr = comImpl_->StartEventMonitoring(isContainerServiceManaged, sinceTime);

    return ErrorCode::FromHResult(hr, true);
}

AsyncOperationSPtr ComProxyContainerActivatorService::BeginActivateContainer(
    ContainerActivationArgs const & activationArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ActivateContainerAsyncOperation>(
        *comImpl_.GetRawPointer(),
        activationArgs,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyContainerActivatorService::EndActivateContainer(
    AsyncOperationSPtr const & operation,
    __out std::wstring & containerId)
{
    return ActivateContainerAsyncOperation::End(operation, containerId);
}

AsyncOperationSPtr ComProxyContainerActivatorService::BeginDeactivateContainer(
    ContainerDeactivationArgs const & deactivationArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DeactivateContainerAsyncOperation>(
        *comImpl_.GetRawPointer(),
        deactivationArgs,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyContainerActivatorService::EndDeactivateContainer(
    AsyncOperationSPtr const & operation)
{
    return DeactivateContainerAsyncOperation::End(operation);
}

AsyncOperationSPtr ComProxyContainerActivatorService::BeginDownloadImages(
    vector<ContainerImageDescription> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        *comImpl_.GetRawPointer(),
        images,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyContainerActivatorService::EndDownloadImages(
    AsyncOperationSPtr const & operation)
{
    return DownloadContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr ComProxyContainerActivatorService::BeginDeleteImages(
    vector<wstring> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteContainerImagesAsyncOperation>(
        *comImpl_.GetRawPointer(),
        images,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyContainerActivatorService::EndDeleteImages(
    AsyncOperationSPtr const & operation)
{
    return DeleteContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr ComProxyContainerActivatorService::BeginInvokeContainerApi(
    ContainerApiExecutionArgs const & apiExecArgs,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<InvokeContainerApiAsyncOperation>(
        *comImpl_.GetRawPointer(),
        apiExecArgs,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyContainerActivatorService::EndInvokeContainerApi(
    AsyncOperationSPtr const & operation,
    __out ContainerApiExecutionResponse & apiExecResp)
{
    return InvokeContainerApiAsyncOperation::End(operation, apiExecResp);
}
