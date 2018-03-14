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

ktl::Awaitable<bool> BackupCallbackHandler::UploadBackupAsync(
    __in TxnReplicator::BackupInfo backupInfo,
    __in ktl::CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);
    Backup_Info backup_info;
    ktl::Awaitable<bool> awaitable;
    NTSTATUS status = STATUS_SUCCESS;
    
    status = ktl::AwaitableCompletionSource<bool>::Create(GetThisAllocator(), RELIABLECOLLECTIONRUNTIME_TAG, acsSPtr_);

    awaitable = acsSPtr_->GetAwaitable();
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
    return awaitable;
}

BackupCallbackHandler::BackupCallbackHandler(
    __in fnUploadAsync uploadAsyncCallback,
    __in void* uploadAsyncCallbackContext)
{
    uploadAsyncCallback_ = uploadAsyncCallback;
    uploadAsyncCallbackContext_ = uploadAsyncCallbackContext;
}

BackupCallbackHandler::~BackupCallbackHandler()
{
}

void BackupCallbackHandler::SetResult(__in bool result)
{
    acsSPtr_->SetResult(result);
}

