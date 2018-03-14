// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupCopier
        {
            class RunBackupCopierExeAsyncOperation : public BackupCopierAsyncOperationBase
            {
            public:
                RunBackupCopierExeAsyncOperation(
                    __in BackupCopierProxy & owner,
                    Common::ActivityId const & activityId,
                    wstring const & operation,
                    wstring const & args,
                    wstring const & argsLogString,
                    TimeSpan const timeout,
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & parent);

                static ErrorCode End(AsyncOperationSPtr const & operation);

                void OnStart(AsyncOperationSPtr const & thisSPtr) override;

                void OnCancel() override;

                void OnProcessJob(AsyncOperationSPtr const &) override;

            private:

                void DoRunBackupCopierExe(AsyncOperationSPtr const & thisSPtr);

                void OnWaitOneComplete(
                    AsyncOperationSPtr const & thisSPtr,
                    ErrorCode const & waitResult,
                    DWORD exitCode);

                wstring operation_;
                wstring args_;
                wstring argsLogString_;
                wstring tempWorkingDirectory_;
                wstring errorDetailsFile_;
                ProcessWaitSPtr processWait_;
                HANDLE hProcess_;
                HANDLE hThread_;
            };
        }
    }
}
