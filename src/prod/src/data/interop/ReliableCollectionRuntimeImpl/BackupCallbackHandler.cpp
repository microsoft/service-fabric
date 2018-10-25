// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

HRESULT BackupCallbackHandler::Create(
    __in KAllocator& allocator,
    __in fnUploadAsync uploadAsyncCallback,
    __in void* uploadAsyncCallbackContext,
    __out BackupCallbackHandler::SPtr& result)
{
    BackupCallbackHandler* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) BackupCallbackHandler(uploadAsyncCallback, uploadAsyncCallbackContext);
    if (!pointer)
        return E_OUTOFMEMORY;

    result = pointer;
    return S_OK;
}

HRESULT BackupCallbackHandler::Create(
    __in KAllocator& allocator,
    __in fnUploadAsync2 uploadAsyncCallback,
    __in void* uploadAsyncCallbackContext,
    __out BackupCallbackHandler::SPtr& result)
{
    BackupCallbackHandler* pointer = _new(RELIABLECOLLECTIONRUNTIME_TAG, allocator) BackupCallbackHandler(uploadAsyncCallback, uploadAsyncCallbackContext);
    if (!pointer)
        return E_OUTOFMEMORY;

    result = pointer;
    return S_OK;
}

ktl::Awaitable<bool> BackupCallbackHandler::UploadBackupAsync(
    __in TxnReplicator::BackupInfo backupInfo,
    __in ktl::CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);
    ktl::Awaitable<bool> awaitable;
    NTSTATUS status = STATUS_SUCCESS;
    
    status = ktl::AwaitableCompletionSource<bool>::Create(GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, acsSPtr_);

    awaitable = acsSPtr_->GetAwaitable();

    // new version : Backup_Info2 which has Size
    // Since in this change there is no previous backup_info that has Size so using newVersion_ way to distinguish old and new BackupCallbackHandler
    if (uploadAsyncCallback2_ != nullptr)
    {
        Backup_Info2 backup_info;
        backup_info.backupId = backupInfo.BackupId.AsGUID();
        backup_info.parentbackupId = backupInfo.ParentBackupId.AsGUID();
        backup_info.directoryPath = static_cast<LPCWSTR>(*backupInfo.Directory);
        backup_info.option = (Backup_Option)backupInfo.get_BackupOption();

        backup_info.version.epoch.DataLossNumber = backupInfo.Version.get_Epoch().DataLossNumber;
        backup_info.version.epoch.ConfigurationNumber = backupInfo.Version.get_Epoch().ConfigurationNumber;
        backup_info.version.epoch.Reserved = backupInfo.Version.get_Epoch().Reserved;
        backup_info.version.lsn = backupInfo.Version.get_LSN();

        backup_info.startVersion.epoch.DataLossNumber = backupInfo.StartVersion.get_Epoch().DataLossNumber;
        backup_info.startVersion.epoch.ConfigurationNumber = backupInfo.StartVersion.get_Epoch().ConfigurationNumber;
        backup_info.startVersion.epoch.Reserved = backupInfo.StartVersion.get_Epoch().Reserved;
        backup_info.startVersion.lsn = backupInfo.StartVersion.get_LSN();

        uploadAsyncCallback2_(
            uploadAsyncCallbackContext_,
            &backup_info,
            sizeof(backup_info),
            [](void* ctx, BOOL result) {
            BackupCallbackHandler* handler = (BackupCallbackHandler*)ctx;
            handler->SetResult(result);
            },
            this);
    }
    else
    {
        Backup_Info backup_info;
        backup_info.backupId = backupInfo.BackupId.AsGUID();
        backup_info.directoryPath = static_cast<LPCWSTR>(*backupInfo.Directory);
        backup_info.option = (Backup_Option)backupInfo.get_BackupOption();

        backup_info.version.epoch.DataLossNumber = backupInfo.Version.get_Epoch().DataLossNumber;
        backup_info.version.epoch.ConfigurationNumber = backupInfo.Version.get_Epoch().ConfigurationNumber;
        backup_info.version.epoch.Reserved = backupInfo.Version.get_Epoch().Reserved;
        backup_info.version.lsn = backupInfo.Version.get_LSN();

        uploadAsyncCallback_(
            uploadAsyncCallbackContext_,
            backup_info,
            [](void* ctx, BOOL result) {
            BackupCallbackHandler* handler = (BackupCallbackHandler*)ctx;
            handler->SetResult(result);
            },
            this);
    }
    
    return awaitable;
}

BackupCallbackHandler::BackupCallbackHandler(
    __in fnUploadAsync uploadAsyncCallback,
    __in void* uploadAsyncCallbackContext)
{
    uploadAsyncCallback_ = uploadAsyncCallback;
    uploadAsyncCallbackContext_ = uploadAsyncCallbackContext;
    uploadAsyncCallback2_ = nullptr;
}

BackupCallbackHandler::BackupCallbackHandler(
    __in fnUploadAsync2 uploadAsyncCallback,
    __in void* uploadAsyncCallbackContext)
{
    uploadAsyncCallback2_ = uploadAsyncCallback;
    uploadAsyncCallbackContext_ = uploadAsyncCallbackContext;
    uploadAsyncCallback_ = nullptr;
}

BackupCallbackHandler::~BackupCallbackHandler()
{
}

void BackupCallbackHandler::SetResult(__in bool result)
{
    acsSPtr_->SetResult(result);
}

