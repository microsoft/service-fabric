// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcReceiverContext : public ReceiverContext
    {
        DENY_COPY_ASSIGNMENT(IpcReceiverContext);

    public:
        IpcReceiverContext(
            IpcHeader && ipcHeader,
            Transport::MessageId const & messageId, 
            ISendTarget::SPtr const & replyTargetSPtr,
            IDatagramTransportSPtr const & datagramTransport);

        __declspec(property(get = getFrom)) std::wstring const & From;
        std::wstring const & getFrom() const { return ipcHeader_.From; }

        __declspec(property(get = getFromProcessId)) DWORD FromProcessId;
        DWORD getFromProcessId() const { return ipcHeader_.FromProcessId; }

    private:
        IpcHeader ipcHeader_;
    };

    typedef std::unique_ptr<IpcReceiverContext> IpcReceiverContextUPtr;
}
