// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

StringLiteral const TraceComponent("ComProxyBackupRestoreHandler");

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyBackupRestoreHandler::UpdateBackupSchedulingPolicyAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(UpdateBackupSchedulingPolicyAsyncOperation)

public:
    UpdateBackupSchedulingPolicyAsyncOperation(
        __in IFabricBackupRestoreHandler & comImpl,
        __in FABRIC_BACKUP_POLICY* policy,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , policy_(policy)
        , timeout_(timeout)
    {
    }

    virtual ~UpdateBackupSchedulingPolicyAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<UpdateBackupSchedulingPolicyAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginUpdateBackupSchedulePolicy(
            policy_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndUpdateBackupSchedulePolicy(context);

    }

    IFabricBackupRestoreHandler & comImpl_;
    FABRIC_BACKUP_POLICY* policy_;
    TimeSpan const timeout_;
};

class ComProxyBackupRestoreHandler::BackupPartitionAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(BackupPartitionAsyncOperation)

public:
    BackupPartitionAsyncOperation(
        __in IFabricBackupRestoreHandler & comImpl,
        __in FABRIC_BACKUP_OPERATION_ID operationId,
        __in FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationId_(operationId)
        , configuration_(backupConfiguration)
        , timeout_(timeout)
    {
    }

    virtual ~BackupPartitionAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<BackupPartitionAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginPartitionBackupOperation(
            operationId_,
            configuration_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndUpdateBackupSchedulePolicy(context);

    }

    IFabricBackupRestoreHandler & comImpl_;
    FABRIC_BACKUP_CONFIGURATION* configuration_;
    FABRIC_BACKUP_OPERATION_ID operationId_;
    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyBackupRestoreHandler Implementation
//
ComProxyBackupRestoreHandler::ComProxyBackupRestoreHandler(
    ComPointer<IFabricBackupRestoreHandler> const & comImpl)
    : IBackupRestoreHandler()
    , comImpl_(comImpl)
{
}

ComProxyBackupRestoreHandler::~ComProxyBackupRestoreHandler()
{
}

AsyncOperationSPtr ComProxyBackupRestoreHandler::BeginUpdateBackupSchedulingPolicy(
    FABRIC_BACKUP_POLICY * policy,
    TimeSpan const timeoutMilliseconds,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<UpdateBackupSchedulingPolicyAsyncOperation>(
        *comImpl_.GetRawPointer(),
        policy,
        timeoutMilliseconds,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreHandler::EndUpdateBackupSchedulingPolicy(
    AsyncOperationSPtr const &operation)
{
    return UpdateBackupSchedulingPolicyAsyncOperation::End(operation);
}

AsyncOperationSPtr ComProxyBackupRestoreHandler::BeginPartitionBackupOperation(
    FABRIC_BACKUP_OPERATION_ID operationId,
    FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<BackupPartitionAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationId,
        backupConfiguration,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyBackupRestoreHandler::EndPartitionBackupOperation(
    AsyncOperationSPtr const &operation)
{
    return BackupPartitionAsyncOperation::End(operation);
}

