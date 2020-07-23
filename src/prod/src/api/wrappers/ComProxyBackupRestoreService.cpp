// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

StringLiteral const TraceComponent("ComProxyBackupRestoreService");

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyBackupRestoreService::GetBackupSchedulingPolicyAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetBackupSchedulingPolicyAsyncOperation)

public:
    GetBackupSchedulingPolicyAsyncOperation(
        __in IFabricBackupRestoreService & comImpl,
        __in FABRIC_BACKUP_PARTITION_INFO* partitionInfo,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , partitionInfo_(partitionInfo)
        , timeout_(timeout)
    {
    }

    virtual ~GetBackupSchedulingPolicyAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricGetBackupSchedulePolicyResult ** reply)
    {
        auto casted = AsyncOperation::End<GetBackupSchedulingPolicyAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetBackupSchedulePolicy(
                partitionInfo_,
                this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetBackupSchedulePolicy(context, (IFabricGetBackupSchedulePolicyResult**)&result);

    }

    IFabricBackupRestoreService & comImpl_;
    FABRIC_BACKUP_PARTITION_INFO* partitionInfo_;
    IFabricGetBackupSchedulePolicyResult * result;
    TimeSpan const timeout_;
};

class ComProxyBackupRestoreService::GetRestorePointDetailsAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetRestorePointDetailsAsyncOperation)

public:
    GetRestorePointDetailsAsyncOperation(
        __in IFabricBackupRestoreService & comImpl,
        __in FABRIC_BACKUP_PARTITION_INFO* partitionInfo,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , partitionInfo_(partitionInfo)
        , timeout_(timeout)
    {
    }

    virtual ~GetRestorePointDetailsAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricGetRestorePointDetailsResult ** reply)
    {
        auto casted = AsyncOperation::End<GetRestorePointDetailsAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetRestorePointDetails(
            partitionInfo_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetRestorePointDetails(context, (IFabricGetRestorePointDetailsResult**)&result);

    }

    IFabricBackupRestoreService & comImpl_;
    FABRIC_BACKUP_PARTITION_INFO* partitionInfo_;
    IFabricGetRestorePointDetailsResult * result;
    TimeSpan const timeout_;
};

class ComProxyBackupRestoreService::ReportBackupOperationResultAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ReportBackupOperationResultAsyncOperation)

public:
    ReportBackupOperationResultAsyncOperation(
        __in IFabricBackupRestoreService & comImpl,
        __in FABRIC_BACKUP_OPERATION_RESULT* operationResult,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationResult_(operationResult)
        , timeout_(timeout)
    {
    }

    virtual ~ReportBackupOperationResultAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<ReportBackupOperationResultAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginReportBackupOperationResult(
            operationResult_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndReportBackupOperationResult(context);

    }

    IFabricBackupRestoreService & comImpl_;
    FABRIC_BACKUP_OPERATION_RESULT* operationResult_;
    TimeSpan const timeout_;
};

class ComProxyBackupRestoreService::ReportRestoreOperationResultAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ReportRestoreOperationResultAsyncOperation)

public:
    ReportRestoreOperationResultAsyncOperation(
        __in IFabricBackupRestoreService & comImpl,
        __in FABRIC_RESTORE_OPERATION_RESULT* operationResult,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationResult_(operationResult)
        , timeout_(timeout)
    {
    }

    virtual ~ReportRestoreOperationResultAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<ReportRestoreOperationResultAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginReportRestoreOperationResult(
            operationResult_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndReportRestoreOperationResult(context);

    }

    IFabricBackupRestoreService & comImpl_;
    FABRIC_RESTORE_OPERATION_RESULT* operationResult_;
    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyBackupRestoreService Implementation
//
ComProxyBackupRestoreService::ComProxyBackupRestoreService(
    ComPointer<IFabricBackupRestoreService> const & comImpl)
    : IBackupRestoreService()
    , comImpl_(comImpl)
{
}

ComProxyBackupRestoreService::~ComProxyBackupRestoreService()
{
}

AsyncOperationSPtr ComProxyBackupRestoreService::BeginGetBackupSchedulingPolicy(
    FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
    TimeSpan const timeoutMilliseconds,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetBackupSchedulingPolicyAsyncOperation>(
        *comImpl_.GetRawPointer(),
        partitionInfo,
        timeoutMilliseconds,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreService::EndGetBackupSchedulingPolicy(
    AsyncOperationSPtr const &operation,
    IFabricGetBackupSchedulePolicyResult ** policyResult)
{
    return GetBackupSchedulingPolicyAsyncOperation::End(operation, policyResult);
}

AsyncOperationSPtr ComProxyBackupRestoreService::BeginGetRestorePointDetails(
    FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
    TimeSpan const timeoutMilliseconds,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetRestorePointDetailsAsyncOperation>(
        *comImpl_.GetRawPointer(),
        partitionInfo,
        timeoutMilliseconds,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreService::EndGetGetRestorePointDetails(
    AsyncOperationSPtr const &operation,
    __out IFabricGetRestorePointDetailsResult ** restorePointResult)
{
    return GetRestorePointDetailsAsyncOperation::End(operation, restorePointResult);
}

AsyncOperationSPtr ComProxyBackupRestoreService::BeginReportBackupOperationResult(
    FABRIC_BACKUP_OPERATION_RESULT * operationResult,
    TimeSpan const timeoutMilliseconds,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<ReportBackupOperationResultAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationResult,
        timeoutMilliseconds,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreService::EndReportBackupOperationResult(
    AsyncOperationSPtr const &operation)
{
    return ReportBackupOperationResultAsyncOperation::End(operation);
}


AsyncOperationSPtr ComProxyBackupRestoreService::BeginReportRestoreOperationResult(
    FABRIC_RESTORE_OPERATION_RESULT * operationResult,
    TimeSpan const timeoutMilliseconds,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<ReportRestoreOperationResultAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationResult,
        timeoutMilliseconds,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreService::EndReportRestoreOperationResult(
    AsyncOperationSPtr const &operation)
{
    return ReportRestoreOperationResultAsyncOperation::End(operation);
}
