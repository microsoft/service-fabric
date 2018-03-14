// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyInfrastructureService::RunCommandAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(RunCommandAsyncOperation)

public:
    RunCommandAsyncOperation(
        bool isAdminCommand,
        __in IFabricInfrastructureService & comImpl,
        wstring const & command,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , command_(command)
        , timeout_(timeout)
        , isAdminCommand_(isAdminCommand)
    {
    }

    virtual ~RunCommandAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, wstring & reply)
    {
        auto casted = AsyncOperation::End<RunCommandAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginRunCommand(
            isAdminCommand_,
            command_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricStringResult> result;
        auto hr = comImpl_.EndRunCommand(context, result.InitializationAddress());

        if (SUCCEEDED(hr))
        {
            hr = StringUtility::LpcwstrToWstring(
                result->get_String(),
                true,
                0,
                ParameterValidator::MaxEndpointSize,
                reply_);
        }

        return hr;
    }

    bool isAdminCommand_;
    IFabricInfrastructureService & comImpl_;
    wstring command_;
    wstring reply_;
    TimeSpan const timeout_;
};

class ComProxyInfrastructureService::ReportTaskAsyncOperationBase : public ComProxyAsyncOperation
{
    DENY_COPY(ReportTaskAsyncOperationBase)

public:
    ReportTaskAsyncOperationBase(
        __in IFabricInfrastructureService & comImpl,
        wstring const & taskId,
        uint64 instanceId,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , taskId_(taskId)
        , instanceId_(instanceId)
        , timeout_(timeout)
    {
    }

    virtual ~ReportTaskAsyncOperationBase() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ReportTaskAsyncOperationBase>(operation)->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    IFabricInfrastructureService & comImpl_;
    wstring taskId_;
    uint64 instanceId_;
    TimeSpan const timeout_;
};

class ComProxyInfrastructureService::ReportStartTaskSuccessAsyncOperation : public ReportTaskAsyncOperationBase
{
    DENY_COPY(ReportStartTaskSuccessAsyncOperation)

public:
    ReportStartTaskSuccessAsyncOperation(
        __in IFabricInfrastructureService & comImpl,
        wstring const & taskId,
        uint64 instanceId,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(comImpl, taskId, instanceId, timeout, callback, parent)
    {
    }

    virtual ~ReportStartTaskSuccessAsyncOperation() { }

protected:
    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginReportStartTaskSuccess(
            taskId_.c_str(),
            instanceId_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndReportStartTaskSuccess(context);
    }
};

class ComProxyInfrastructureService::ReportFinishTaskSuccessAsyncOperation : public ReportTaskAsyncOperationBase
{
    DENY_COPY(ReportFinishTaskSuccessAsyncOperation)

public:
    ReportFinishTaskSuccessAsyncOperation(
        __in IFabricInfrastructureService & comImpl,
        wstring const & taskId,
        uint64 instanceId,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(comImpl, taskId, instanceId, timeout, callback, parent)
    {
    }

    virtual ~ReportFinishTaskSuccessAsyncOperation() { }

protected:
    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginReportFinishTaskSuccess(
            taskId_.c_str(),
            instanceId_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndReportFinishTaskSuccess(context);
    }
};

class ComProxyInfrastructureService::ReportTaskFailureAsyncOperation : public ReportTaskAsyncOperationBase
{
    DENY_COPY(ReportTaskFailureAsyncOperation)

public:
    ReportTaskFailureAsyncOperation(
        __in IFabricInfrastructureService & comImpl,
        wstring const & taskId,
        uint64 instanceId,
        TimeSpan const & timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : ReportTaskAsyncOperationBase(comImpl, taskId, instanceId, timeout, callback, parent)
    {
    }

    virtual ~ReportTaskFailureAsyncOperation() { }

protected:
    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginReportTaskFailure(
            taskId_.c_str(),
            instanceId_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndReportTaskFailure(context);
    }
};

// ********************************************************************************************************************
// ComProxyInfrastructureService Implementation
//
ComProxyInfrastructureService::ComProxyInfrastructureService(
    ComPointer<IFabricInfrastructureService> const & comImpl)
    : IInfrastructureService()
    , comImpl_(comImpl)
{
}

ComProxyInfrastructureService::~ComProxyInfrastructureService()
{
}

AsyncOperationSPtr ComProxyInfrastructureService::BeginRunCommand(
    bool isAdminCommand,
    wstring const & command,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<RunCommandAsyncOperation>(
        isAdminCommand,
        *comImpl_.GetRawPointer(),
        command,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyInfrastructureService::EndRunCommand(
    AsyncOperationSPtr const & asyncOperation,
    wstring & reply)
{
    return RunCommandAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr ComProxyInfrastructureService::BeginReportStartTaskSuccess(
    wstring const & taskId,
    uint64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ReportStartTaskSuccessAsyncOperation>(
        *comImpl_.GetRawPointer(),
        taskId,
        instanceId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyInfrastructureService::EndReportStartTaskSuccess(
    AsyncOperationSPtr const & asyncOperation)
{
    return ReportStartTaskSuccessAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyInfrastructureService::BeginReportFinishTaskSuccess(
    wstring const & taskId,
    uint64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ReportFinishTaskSuccessAsyncOperation>(
        *comImpl_.GetRawPointer(),
        taskId,
        instanceId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyInfrastructureService::EndReportFinishTaskSuccess(
    AsyncOperationSPtr const & asyncOperation)
{
    return ReportFinishTaskSuccessAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyInfrastructureService::BeginReportTaskFailure(
    wstring const & taskId,
    uint64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ReportTaskFailureAsyncOperation>(
        *comImpl_.GetRawPointer(),
        taskId,
        instanceId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyInfrastructureService::EndReportTaskFailure(
    AsyncOperationSPtr const & asyncOperation)
{
    return ReportTaskFailureAsyncOperation::End(asyncOperation);
}
