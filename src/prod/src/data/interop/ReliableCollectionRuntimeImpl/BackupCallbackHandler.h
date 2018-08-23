// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
class BackupCallbackHandler
    : public KObject<BackupCallbackHandler>
    , public KShared<BackupCallbackHandler>
    , public TxnReplicator::IBackupCallbackHandler
{
    K_FORCE_SHARED(BackupCallbackHandler);
    K_SHARED_INTERFACE_IMP(IBackupCallbackHandler);

public:
    static HRESULT Create(
        __in KAllocator& allocator,
        __in fnUploadAsync uploadAsyncCallback,
        __in void* uploadAsyncCallbackContext_,
        __out BackupCallbackHandler::SPtr& result);

    static HRESULT Create(
        __in KAllocator& allocator,
        __in fnUploadAsync2 uploadAsyncCallback,
        __in void* uploadAsyncCallbackContext_,
        __out BackupCallbackHandler::SPtr& result);

    ktl::Awaitable<bool> UploadBackupAsync(
        __in TxnReplicator::BackupInfo backupInfo,
        __in ktl::CancellationToken const & cancellationToken);
private:
    BackupCallbackHandler(
        __in fnUploadAsync uploadAsyncCallback,
        __in void* uploadAsyncCallbackContext);

    BackupCallbackHandler(
        __in fnUploadAsync2 uploadAsyncCallback,
        __in void* uploadAsyncCallbackContext);

    void SetResult(__in bool result);
    fnUploadAsync uploadAsyncCallback_;
    fnUploadAsync2 uploadAsyncCallback2_;
    void* uploadAsyncCallbackContext_;
    ktl::AwaitableCompletionSource<bool>::SPtr acsSPtr_;
};

