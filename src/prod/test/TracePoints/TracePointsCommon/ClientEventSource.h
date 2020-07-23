// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class ClientEventSource : private DenyCopy
    {
    public:
        ClientEventSource();
        ~ClientEventSource();

        void PipeClientConnected(int id) const;
        void PipeClientConnectFailed(DWORD error) const;
        void PipeSent(int bytesWritten) const;
        void PipeSendFailed(DWORD error) const;
        void PipeClientClosed() const;
        void CreateCommandFailed(DWORD error) const;
        void SendingSetupCommand(TracePointCommand::Data const & command) const;
        void SendingInvokeCommand(TracePointCommand::Data const & command) const;
        void SendingCleanupCommand(TracePointCommand::Data const & command) const;
        void SendCommandFailed(DWORD error) const;

    private:
        static GUID const TracePointsProviderId;
        REGHANDLE regHandle_;

        static REGHANDLE RegisterProvider();

        void WriteEvent(USHORT id, UCHAR level) const;
        void WriteEvent(USHORT id, UCHAR level, std::vector<EVENT_DATA_DESCRIPTOR> & userData) const;
        void WriteCommandEvent(USHORT id, UCHAR level, TracePointCommand::Data const & command) const;

        template <typename T1>
        void WriteEvent(USHORT id, UCHAR level, T1 data1) const
        {
            vector<EVENT_DATA_DESCRIPTOR> userData(1);
            EVENT_DATA_DESCRIPTOR item1;
            EventDataDescCreate(&item1, &data1, sizeof(data1));
            userData.push_back(item1);
            WriteEvent(id, level, userData);
        }
    };
}
