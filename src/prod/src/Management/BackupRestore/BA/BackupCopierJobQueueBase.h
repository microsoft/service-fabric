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
            class BackupCopierJobQueueBase : public JobQueue<BackupCopierJobItem, ComponentRoot>
            {
            public:
                BackupCopierJobQueueBase(
                    wstring const & name,
                    __in ComponentRoot & root);

                virtual void TryEnqueueOrFail(__in BackupCopierProxy & owner, AsyncOperationSPtr const & operation);

                void UnregisterAndClose();

            protected:

                virtual ConfigEntryBase & GetThrottleConfigEntry() = 0;
                virtual int GetThrottleConfigValue() = 0;

                void InitializeConfigUpdates();

            private:
                void OnConfigUpdate();

                void OnJobQueueItemComplete();

                HHandler configHandlerId_;
            };
        }
    }
}
