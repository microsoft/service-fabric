// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class IMessageHandler
        {
        public:
            virtual void ProcessIpcMessage(Transport::MessageUPtr && message, Transport::IpcReceiverContextUPtr && receiverContext) = 0;

            virtual ~IMessageHandler() {}
        };
    }
}
