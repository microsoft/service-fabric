// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore;
    class OpenLocalStoreAsyncOperation;

    class OpenLocalStoreJobQueue : public Common::JobQueue<std::shared_ptr<OpenLocalStoreAsyncOperation>, Common::ComponentRoot>
    {
    public:

        static OpenLocalStoreJobQueue & GetGlobalQueue();

        Common::AsyncOperationSPtr BeginEnqueueOpenLocalStore(
            __in ReplicatedStore &,
            bool databaseShouldExist,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndEnqueueOpenLocalStore(
            Common::AsyncOperationSPtr const &);

    private:
        static BOOL CALLBACK CreateGlobalQueue(PINIT_ONCE, PVOID, PVOID *);
        static Common::Global<OpenLocalStoreJobQueue> GlobalQueue;

    private:
        friend class OpenLocalStoreAsyncOperation;

        OpenLocalStoreJobQueue(
            std::wstring const & name,
            __in Common::ComponentRoot & root);

        bool TryEnqueue(__in ReplicatedStore &, Common::AsyncOperationSPtr const & operation);
        void UnregisterAndClose();

        void OnConfigUpdate();

        Common::HHandler configHandlerId_;
    };

    class OpenLocalStoreAsyncOperation : public Common::AsyncOperation
    {
    public:
        OpenLocalStoreAsyncOperation(
            __in OpenLocalStoreJobQueue & jobQueue,
            __in ReplicatedStore & store,
            bool databaseShouldExist,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        void OnStart(Common::AsyncOperationSPtr const &) override;
        void OnCancel() override;

        bool ProcessJob(Common::ComponentRoot &);

    private:
        OpenLocalStoreJobQueue & jobQueue_;
        ReplicatedStore & store_;
        bool databaseShouldExist_;
    };
}
