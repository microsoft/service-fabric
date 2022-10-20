// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // {c7ce7bf8-0226-412e-bb4b-c6f219375180}
    static const GUID CLSID_ComFabricBackupRestoreAgent =
    { 0xc7ce7bf8, 0x0226, 0x412e,{ 0xbb, 0x4b, 0xc6, 0xf2, 0x19, 0x37, 0x51, 0x80 } };

    class ComFabricBackupRestoreAgent :
        public IFabricBackupRestoreAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricBackupRestoreAgent);

        BEGIN_COM_INTERFACE_LIST(ComFabricBackupRestoreAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricBackupRestoreAgent)
            COM_INTERFACE_ITEM(IID_IFabricBackupRestoreAgent, IFabricBackupRestoreAgent)
            COM_INTERFACE_ITEM(CLSID_ComFabricBackupRestoreAgent, ComFabricBackupRestoreAgent)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricBackupRestoreAgent(FabricBackupRestoreAgentImplSPtr const & fabricRuntime);
        virtual ~ComFabricBackupRestoreAgent();

        HRESULT STDMETHODCALLTYPE RegisterBackupRestoreReplica(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [in] */ IFabricBackupRestoreHandler *backupRestoreHandler);

        HRESULT STDMETHODCALLTYPE UnregisterBackupRestoreReplica(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId);

        HRESULT STDMETHODCALLTYPE BeginGetBackupSchedulePolicy(
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetBackupSchedulePolicy(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policy);

        HRESULT STDMETHODCALLTYPE BeginGetRestorePointDetails(
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetRestorePointDetails(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetRestorePointDetailsResult **restorePointDetails);

        HRESULT STDMETHODCALLTYPE BeginReportBackupOperationResult(
            /* [in] */ FABRIC_BACKUP_OPERATION_RESULT *backupOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndReportBackupOperationResult(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginReportRestoreOperationResult(
            /* [in] */ FABRIC_RESTORE_OPERATION_RESULT *operationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndReportRestoreOperationResult(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUploadBackup(
            /* [in] */ FABRIC_BACKUP_UPLOAD_INFO *uploadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndUploadBackup(
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE BeginDownloadBackup(
            /* [in] */ FABRIC_BACKUP_DOWNLOAD_INFO *downloadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndDownloadBackup(
            /* [in] */ IFabricAsyncOperationContext *context);

        __declspec(property(get = get_BackupRestoreAgent)) FabricBackupRestoreAgentImplSPtr const & BackupRestoreAgent;
        inline FabricBackupRestoreAgentImplSPtr const & get_BackupRestoreAgent() const { return fabricBackupRestoreAgent_; }

    private:
        class GetBackupPolicyComAsyncOperationContext;
        class GetRestorePointDetailsComAsyncOperationContext;
        class ReportBackupOperationResultComAsyncOperationContext;
        class ReportRestoreOperationResultComAsyncOperationContext;
        class UploadBackupComAsyncOperationContext;
        class DownloadBackupComAsyncOperationContext;

    private:
        FabricBackupRestoreAgentImplSPtr const fabricBackupRestoreAgent_;
    };
}
