// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

// ********************************************************************************************************************
// ComBackupRestoreServiceAgent::UpdateBackupSchedulePolicyComAsyncOperationContext Implementation
//

// {c6ec8493-c3ea-4334-bc5c-737dc6ba8f32}
static const GUID CLSID_UpdateBackupSchedulePolicyComAsyncOperationContext =
{ 0xc6ec8493, 0xc3ea, 0x4334,{ 0xbc, 0x5c, 0x73, 0x7d, 0xc6, 0xba, 0x8f, 0x32 } };

class ComBackupRestoreServiceAgent::UpdateBackupSchedulePolicyComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(UpdateBackupSchedulePolicyComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            UpdateBackupSchedulePolicyComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_UpdateBackupSchedulePolicyComAsyncOperationContext,
            UpdateBackupSchedulePolicyComAsyncOperationContext)

public:
    explicit UpdateBackupSchedulePolicyComAsyncOperationContext(__in ComBackupRestoreServiceAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~UpdateBackupSchedulePolicyComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_PARTITION_INFO* partitionInfo,
        __in FABRIC_BACKUP_POLICY* policy,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (partitionInfo == NULL) { return E_POINTER; }
        if (!ParameterValidator::IsValid(partitionInfo->ServiceName).IsSuccess()) { return E_INVALIDARG; }

        partitionInfo_ = partitionInfo;
        policy_ = policy;

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UpdateBackupSchedulePolicyComAsyncOperationContext> thisOperation(context, CLSID_UpdateBackupSchedulePolicyComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginUpdateBackupSchedulePolicy(
            partitionInfo_,
            policy_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishUpdateBackupSchedulePolicy(operation, false); },
            proxySPtr);
        FinishUpdateBackupSchedulePolicy(operation, true);
    }

private:
    void FinishUpdateBackupSchedulePolicy(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.impl_->EndUpdateBackupSchedulePolicy(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComBackupRestoreServiceAgent & owner_;
    TimeSpan timeout_;
    FABRIC_BACKUP_PARTITION_INFO * partitionInfo_;
    FABRIC_BACKUP_POLICY* policy_;
};

// ********************************************************************************************************************
// ComBackupRestoreServiceAgent::BackupPartitionComAsyncOperationContext Implementation
//

// {21305d04-760d-456c-9752-6ba6f273dd46}
static const GUID CLSID_BackupPartitionComAsyncOperationContext =
{ 0x21305d04, 0x760d, 0x456c,{ 0x97, 0x52, 0x6b, 0xa6, 0xf2, 0x73, 0xdd, 0x46 } };

class ComBackupRestoreServiceAgent::BackupPartitionComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(BackupPartitionComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            BackupPartitionComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_BackupPartitionComAsyncOperationContext,
            BackupPartitionComAsyncOperationContext)

public:
    explicit BackupPartitionComAsyncOperationContext(__in ComBackupRestoreServiceAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~BackupPartitionComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_PARTITION_INFO* partitionInfo,
        __in FABRIC_BACKUP_OPERATION_ID operationId,
        __in FABRIC_BACKUP_CONFIGURATION* configuration,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (partitionInfo == NULL || configuration == NULL) { return E_POINTER; }
        if (!ParameterValidator::IsValid(partitionInfo->ServiceName).IsSuccess()) { return E_INVALIDARG; }
        
        partitionInfo_ = partitionInfo;
        operationId_ = operationId;
        configuration_ = configuration;

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<BackupPartitionComAsyncOperationContext> thisOperation(context, CLSID_BackupPartitionComAsyncOperationContext);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.impl_->BeginPartitionBackupOperation(
            partitionInfo_,
            operationId_,
            configuration_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->EndBackupPartitionRequest(operation, false); },
            proxySPtr);
        EndBackupPartitionRequest(operation, true);
    }

private:
    void EndBackupPartitionRequest(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.impl_->EndPartitionBackupOperation(operation);
        TryComplete(operation->Parent, error.ToHResult());
    }

private:
    ComBackupRestoreServiceAgent & owner_;
    TimeSpan timeout_;
    FABRIC_BACKUP_PARTITION_INFO * partitionInfo_;
    FABRIC_BACKUP_CONFIGURATION* configuration_;
    FABRIC_BACKUP_OPERATION_ID operationId_;
};


// ********************************************************************************************************************
// ComBackupRestoreServiceAgent::ComBackupRestoreServiceAgent Implementation
//

ComBackupRestoreServiceAgent::ComBackupRestoreServiceAgent(IBackupRestoreServiceAgentPtr const & impl)
    : IFabricBackupRestoreServiceAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComBackupRestoreServiceAgent::~ComBackupRestoreServiceAgent()
{
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::RegisterBackupRestoreService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricBackupRestoreService * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterBackupRestoreService(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::UnregisterBackupRestoreService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterBackupRestoreService(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::BeginUpdateBackupSchedulePolicy(
    FABRIC_BACKUP_PARTITION_INFO *info,
    FABRIC_BACKUP_POLICY *policy,
    DWORD timeoutMilliseconds,
    IFabricAsyncOperationCallback *callback,
    IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, rootCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<UpdateBackupSchedulePolicyComAsyncOperationContext> operation
        = make_com<UpdateBackupSchedulePolicyComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        info,
        policy,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::EndUpdateBackupSchedulePolicy(
    IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateBackupSchedulePolicyComAsyncOperationContext::End(context));
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::BeginPartitionBackupOperation(
    FABRIC_BACKUP_PARTITION_INFO *info,
    FABRIC_BACKUP_OPERATION_ID operationId,
    FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
    DWORD timeoutMilliseconds,
    IFabricAsyncOperationCallback *callback,
    IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, rootCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<BackupPartitionComAsyncOperationContext> operation
        = make_com<BackupPartitionComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        info,
        operationId,
        backupConfiguration,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComBackupRestoreServiceAgent::EndPartitionBackupOperation(
    IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(BackupPartitionComAsyncOperationContext::End(context));
}
