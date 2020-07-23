// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "Management/BackupRestoreService/BackupRestoreServiceConfig.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management::BackupRestoreService;
using namespace Management::BackupRestoreAgentComponent;

// {80adfbf4-f45b-45ba-ab5b-21d837da7cad}
static const GUID CLSID_GetBackupPolicyComAsyncOperationContext =
{ 0x80adfbf4, 0xf45b, 0x45ba,{ 0xab, 0x5b, 0x21, 0xd8, 0x37, 0xda, 0x7c, 0xad } };

class ComFabricBackupRestoreAgent::GetBackupPolicyComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(GetBackupPolicyComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            GetBackupPolicyComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_GetBackupPolicyComAsyncOperationContext,
            GetBackupPolicyComAsyncOperationContext)

public:
    explicit GetBackupPolicyComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_(),
        policyResult_()
    {
    }

    virtual ~GetBackupPolicyComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_PARTITION_INFO const* info,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().ApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        partitionInfo_ = info;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context,
        __out void ** policyResult)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetBackupPolicyComAsyncOperationContext> thisOperation(context, CLSID_GetBackupPolicyComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        if (FAILED(hr)) { return hr; }
        thisOperation->policyResult_->QueryInterface(IID_IFabricGetBackupSchedulePolicyResult, policyResult);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginGetPolicyRequest(
            wstring(partitionInfo_->ServiceName),
            Guid(partitionInfo_->PartitionId),
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishGetPolicyRequest(operation, false); },
            proxySPtr);

        FinishGetPolicyRequest(operation, true);
    }

private:
    void FinishGetPolicyRequest(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        BackupPolicy policy;
        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndGetPolicyRequest(operation, policy);
        
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        
        FabricGetBackupSchedulePolicyResultImplSPtr result = make_shared<FabricGetBackupSchedulePolicyResult>(policy);
        policyResult_ = make_com<ComFabricGetBackupSchedulePolicyResult>(result);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_BACKUP_PARTITION_INFO const* partitionInfo_;
    ComPointer<ComFabricGetBackupSchedulePolicyResult> policyResult_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// {8af21080-2ac8-427c-b030-036f4aeb45d1}
static const GUID CLSID_GetRestorePointDetailsComAsyncOperationContext =
{ 0x8af21080, 0x2ac8, 0x427c,{ 0xb0, 0x30, 0x03, 0x6f, 0x4a, 0xeb, 0x45, 0xd1 } };

class ComFabricBackupRestoreAgent::GetRestorePointDetailsComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(GetRestorePointDetailsComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            GetRestorePointDetailsComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_GetRestorePointDetailsComAsyncOperationContext,
            GetRestorePointDetailsComAsyncOperationContext)

public:
    explicit GetRestorePointDetailsComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_(),
        restorePointDetailsResult_()
    {
    }

    virtual ~GetRestorePointDetailsComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_PARTITION_INFO const* info,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().ApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        partitionInfo_ = info;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context,
        __out void ** restorePointDetailsResult)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<GetRestorePointDetailsComAsyncOperationContext> thisOperation(context, CLSID_GetRestorePointDetailsComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        if (FAILED(hr)) { return hr; }
        thisOperation->restorePointDetailsResult_->QueryInterface(IID_IFabricGetRestorePointDetailsResult, restorePointDetailsResult);
        return thisOperation->Result;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginGetRestorePointDetailsRequest(
            wstring(partitionInfo_->ServiceName),
            Guid(partitionInfo_->PartitionId),
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishGetRestorePointDetails(operation, false); },
            proxySPtr);

        FinishGetRestorePointDetails(operation, true);
    }

private:
    void FinishGetRestorePointDetails(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        RestorePointDetails restorePoint;
        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndGetRestorePointDetailsRequest(operation, restorePoint);
        
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        FabricGetRestorePointDetailsResultImplSPtr result = make_shared<FabricGetRestorePointDetailsResult>(restorePoint);
        restorePointDetailsResult_ = make_com<ComFabricGetRestorePointDetailsResult>(result);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_BACKUP_PARTITION_INFO const* partitionInfo_;
    ComPointer<ComFabricGetRestorePointDetailsResult> restorePointDetailsResult_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// {f72c1996-7a01-4fbc-8c24-08c8a2850f8f}
static const GUID CLSID_ReportBackupOperationResultComAsyncOperationContext =
{ 0xf72c1996, 0x7a01, 0x4fbc,{ 0x8c, 0x24, 0x08, 0xc8, 0xa2, 0x85, 0x0f, 0x8f } };

class ComFabricBackupRestoreAgent::ReportBackupOperationResultComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(ReportBackupOperationResultComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            ReportBackupOperationResultComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_ReportBackupOperationResultComAsyncOperationContext,
            ReportBackupOperationResultComAsyncOperationContext)

public:
    explicit ReportBackupOperationResultComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~ReportBackupOperationResultComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_OPERATION_RESULT const* operationResult,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().ApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        operationResult_ = operationResult;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportBackupOperationResultComAsyncOperationContext> thisOperation(context, CLSID_ReportBackupOperationResultComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        return hr;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginReportBackupOperationResult(
            *operationResult_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishReportBackupOperationResult(operation, false); },
            proxySPtr);

        FinishReportBackupOperationResult(operation, true);
    }

private:
    void FinishReportBackupOperationResult(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndReportBackupOperationResult(operation);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_BACKUP_OPERATION_RESULT const* operationResult_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// {d6943714-8353-45f8-8a48-cc0d55a4e470}
static const GUID CLSID_ReportRestoreOperationResultComAsyncOperationContext =
{ 0xd6943714, 0x8353, 0x45f8,{ 0x8a, 0x48, 0xcc, 0x0d, 0x55, 0xa4, 0xe4, 0x70 } };

class ComFabricBackupRestoreAgent::ReportRestoreOperationResultComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(ReportRestoreOperationResultComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            ReportRestoreOperationResultComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_ReportRestoreOperationResultComAsyncOperationContext,
            ReportRestoreOperationResultComAsyncOperationContext)

public:
    explicit ReportRestoreOperationResultComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~ReportRestoreOperationResultComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_RESTORE_OPERATION_RESULT const* operationResult,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().ApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        operationResult_ = operationResult;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ReportRestoreOperationResultComAsyncOperationContext> thisOperation(context, CLSID_ReportRestoreOperationResultComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        return hr;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginReportRestoreOperationResult(
            *operationResult_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishReportRestoreOperationResult(operation, false); },
            proxySPtr);

        FinishReportRestoreOperationResult(operation, true);
    }

private:
    void FinishReportRestoreOperationResult(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndReportRestoreOperationResult(operation);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_RESTORE_OPERATION_RESULT const* operationResult_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// {c8590a6a-fdf9-4f78-a62d-8bd6143b7617}
static const GUID CLSID_UploadBackupComAsyncOperationContext =
{ 0xc8590a6a, 0xfdf9, 0x4f78,{ 0xa6, 0x2d, 0x8b, 0xd6, 0x14, 0x3b, 0x76, 0x17 } };

class ComFabricBackupRestoreAgent::UploadBackupComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(UploadBackupComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            UploadBackupComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_UploadBackupComAsyncOperationContext,
            UploadBackupComAsyncOperationContext)

public:
    explicit UploadBackupComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~UploadBackupComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_UPLOAD_INFO const* uploadInfo,
        __in FABRIC_BACKUP_STORE_INFORMATION const* storeInfo,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().StoreApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        uploadInfo_ = uploadInfo;
        storeInfo_ = storeInfo;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<UploadBackupComAsyncOperationContext> thisOperation(context, CLSID_UploadBackupComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        return hr;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginUploadBackup(
            *uploadInfo_,
            *storeInfo_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishUploadBackup(operation, false); },
            proxySPtr);

        FinishUploadBackup(operation, true);
    }

private:
    void FinishUploadBackup(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndUploadBackup(operation);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_BACKUP_UPLOAD_INFO const* uploadInfo_;
    FABRIC_BACKUP_STORE_INFORMATION const* storeInfo_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// {ac665067-26ce-4c4b-a568-a2635e8c3c89}
static const GUID CLSID_DownloadBackupComAsyncOperationContext =
{ 0xac665067, 0x26ce, 0x4c4b,{ 0xa5, 0x68, 0xa2, 0x63, 0x5e, 0x8c, 0x3c, 0x89 } };

class ComFabricBackupRestoreAgent::DownloadBackupComAsyncOperationContext
    : public ComAsyncOperationContext
{
    DENY_COPY(DownloadBackupComAsyncOperationContext)

        COM_INTERFACE_LIST2(
            DownloadBackupComAsyncOperationContext,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_DownloadBackupComAsyncOperationContext,
            DownloadBackupComAsyncOperationContext)

public:
    explicit DownloadBackupComAsyncOperationContext(__in ComFabricBackupRestoreAgent & owner)
        : ComAsyncOperationContext(),
        owner_(owner),
        timeout_()
    {
    }

    virtual ~DownloadBackupComAsyncOperationContext()
    {
    }

    HRESULT STDMETHODCALLTYPE Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in FABRIC_BACKUP_DOWNLOAD_INFO const* downloadInfo,
        __in FABRIC_BACKUP_STORE_INFORMATION const* storeInfo,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }

        if (timeoutMilliseconds == INFINITE)
        {
            timeout_ = TimeSpan::MaxValue;
        }
        else if (timeoutMilliseconds == 0)
        {
            timeout_ = Common::TimeSpan::FromSeconds(BackupRestoreServiceConfig::GetConfig().StoreApiTimeoutInSeconds);
        }
        else
        {
            timeout_ = TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
        }

        downloadInfo_ = downloadInfo;
        storeInfo_ = storeInfo;

        return S_OK;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<DownloadBackupComAsyncOperationContext> thisOperation(context, CLSID_DownloadBackupComAsyncOperationContext);
        auto hr = thisOperation->ComAsyncOperationContext::End();
        return hr;
    }

protected:
    void OnStart(__in AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.BeginDownloadBackup(
            *downloadInfo_,
            *storeInfo_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->FinishDownloadBackup(operation, false); },
            proxySPtr);

        FinishDownloadBackup(operation, true);
    }

private:
    void FinishDownloadBackup(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        Common::ErrorCode error = owner_.BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.EndDownloadBackup(operation);
        TryComplete(operation->Parent, error);
    }

private:
    FABRIC_BACKUP_DOWNLOAD_INFO const* downloadInfo_;
    FABRIC_BACKUP_STORE_INFORMATION const* storeInfo_;
    ComFabricBackupRestoreAgent & owner_;
    TimeSpan timeout_;
};

// ********************************************************************************************************************
// ComFabricBackupRestoreAgent Implementation
//

ComFabricBackupRestoreAgent::ComFabricBackupRestoreAgent(FabricBackupRestoreAgentImplSPtr const & FabricBackupRestoreAgent)
    : IFabricBackupRestoreAgent(),
    ComUnknownBase(),
    fabricBackupRestoreAgent_(FabricBackupRestoreAgent)
{
}

ComFabricBackupRestoreAgent::~ComFabricBackupRestoreAgent()
{   
    TraceNoise(
        TraceTaskCodes::Hosting,
        "ComFabricBackupRestoreAgent",
        "ComFabricBackupRestoreAgent::Destructed");
}

HRESULT ComFabricBackupRestoreAgent::RegisterBackupRestoreReplica(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ IFabricBackupRestoreHandler *backupRestoreHandler)
{
    if (backupRestoreHandler == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto error = this->BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.RegisterBackupRestoreReplica(
        partitionId,
        replicaId,
        Api::WrapperFactory::create_rooted_com_proxy(backupRestoreHandler));

    return ComUtility::OnPublicApiReturn(error);
}

HRESULT ComFabricBackupRestoreAgent::UnregisterBackupRestoreReplica(
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId)
{
    auto error = this->BackupRestoreAgent->Host.BackupRestoreAgentProxyObj.UnregisterBackupRestoreReplica(
        partitionId,
        replicaId);

    return ComUtility::OnPublicApiReturn(error);
}

HRESULT ComFabricBackupRestoreAgent::BeginGetBackupSchedulePolicy(
    /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<GetBackupPolicyComAsyncOperationContext> operation
        = make_com<GetBackupPolicyComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        info,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndGetBackupSchedulePolicy(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policy)
{
    return ComUtility::OnPublicApiReturn(GetBackupPolicyComAsyncOperationContext::End(context, (void**)policy));
}

HRESULT ComFabricBackupRestoreAgent::BeginGetRestorePointDetails(
    /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<GetRestorePointDetailsComAsyncOperationContext> operation
        = make_com<GetRestorePointDetailsComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        info,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndGetRestorePointDetails(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricGetRestorePointDetailsResult **restorePointDetails)
{
    return ComUtility::OnPublicApiReturn(GetRestorePointDetailsComAsyncOperationContext::End(context, (void**)restorePointDetails));
}

HRESULT ComFabricBackupRestoreAgent::BeginReportBackupOperationResult(
    /* [in] */ FABRIC_BACKUP_OPERATION_RESULT *backupOperationResult,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<ReportBackupOperationResultComAsyncOperationContext> operation
        = make_com<ReportBackupOperationResultComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        backupOperationResult,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndReportBackupOperationResult(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ReportBackupOperationResultComAsyncOperationContext::End(context));
}

HRESULT ComFabricBackupRestoreAgent::BeginReportRestoreOperationResult(
    /* [in] */ FABRIC_RESTORE_OPERATION_RESULT *operationResult,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<ReportRestoreOperationResultComAsyncOperationContext> operation
        = make_com<ReportRestoreOperationResultComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        operationResult,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndReportRestoreOperationResult(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(ReportRestoreOperationResultComAsyncOperationContext::End(context));
}

HRESULT ComFabricBackupRestoreAgent::BeginUploadBackup(
    /* [in] */ FABRIC_BACKUP_UPLOAD_INFO *uploadInfo,
    /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<UploadBackupComAsyncOperationContext> operation
        = make_com<UploadBackupComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        uploadInfo,
        storeInfo,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndUploadBackup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UploadBackupComAsyncOperationContext::End(context));
}

HRESULT ComFabricBackupRestoreAgent::BeginDownloadBackup(
    /* [in] */ FABRIC_BACKUP_DOWNLOAD_INFO *downloadInfo,
    /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<DownloadBackupComAsyncOperationContext> operation
        = make_com<DownloadBackupComAsyncOperationContext>(*this);
    hr = operation->Initialize(
        root,
        downloadInfo,
        storeInfo,
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricBackupRestoreAgent::EndDownloadBackup(
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(DownloadBackupComAsyncOperationContext::End(context));
}
