// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {c90cd4d6-f870-4c7e-8ab2-7c10e039879c}
    static const GUID CLSID_ComBackupRestoreServiceAgent = 
        {0xc90cd4d6,0xf870,0x4c7e,{0x8a,0xb2,0x7c,0x10,0xe0,0x39,0x87,0x9c}};

    class ComBackupRestoreServiceAgent :
        public IFabricBackupRestoreServiceAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComBackupRestoreServiceAgent)

        BEGIN_COM_INTERFACE_LIST(ComBackupRestoreServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricBackupRestoreServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricBackupRestoreServiceAgent, IFabricBackupRestoreServiceAgent)
            COM_INTERFACE_ITEM(CLSID_ComBackupRestoreServiceAgent, ComBackupRestoreServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComBackupRestoreServiceAgent(IBackupRestoreServiceAgentPtr const & impl);
        virtual ~ComBackupRestoreServiceAgent();

        IBackupRestoreServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricBackupRestoreServiceAgent methods
        //

        HRESULT STDMETHODCALLTYPE RegisterBackupRestoreService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricBackupRestoreService *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterBackupRestoreService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

        HRESULT STDMETHODCALLTYPE BeginUpdateBackupSchedulePolicy(
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_POLICY *policy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateBackupSchedulePolicy(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginPartitionBackupOperation(
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_OPERATION_ID operationId,
            /* [in] */ FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPartitionBackupOperation(
            /* [in] */ IFabricAsyncOperationContext *context);

    private:
        class UpdateBackupSchedulePolicyComAsyncOperationContext;
        class BackupPartitionComAsyncOperationContext;

        IBackupRestoreServiceAgentPtr impl_;
    };
}

