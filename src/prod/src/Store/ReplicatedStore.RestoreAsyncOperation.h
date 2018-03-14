// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::RestoreAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(RestoreAsyncOperation);
        
    public:
        RestoreAsyncOperation(
            __in ReplicatedStore & owner,
            std::wstring const & backupDir,
            RestoreSettings const & restoreSettings,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
            , owner_(owner)
            , backupDir_(backupDir)
            , restoreSettings_(restoreSettings)
        {
        }

        virtual ~RestoreAsyncOperation()
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

    protected:
        void OnStart(
            Common::AsyncOperationSPtr const & proxySPtr) override;

    private:		

        void DoRestore(Common::AsyncOperationSPtr const & operation);

        ReplicatedStore & owner_;		
        std::wstring backupDir_;
        RestoreSettings restoreSettings_;
    };
}
