// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    typedef ULONG (WINAPI * EventWriteFunc)(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData);

    class ServerEventSource : private DenyCopy
    {
    public:
        ServerEventSource(REGHANDLE regHandle, EventWriteFunc eventWrite);
        ~ServerEventSource();

        void PipeCreated(int id) const;
        void PipeCreateFailed(DWORD error) const;
        void PipeConnected() const;
        void PipeConnectFailed(DWORD error) const;
        void PipeReceived(int bytesRead) const;
        void PipeReceiveFailed(DWORD error) const;
        void PipeEnded(DWORD error) const;
        void PipeDisconnected() const;
        void PipeDisconnectFailed(DWORD error) const;
        void PipeClosed() const;
        void ServerStarted() const;
        void ServerStartFailed(DWORD error) const;
        void ServerStopped() const;
        void ServerWaitForThreadFailed(DWORD error) const;
        void ReceivedSetupCommand(TracePointCommand::Data const & command) const;
        void ReceivedInvokeCommand(TracePointCommand::Data const & command) const;
        void ReceivedCleanupCommand(TracePointCommand::Data const & command) const;
        void ReceivedUnknownCommand(TracePointCommand::Data const & command) const;
        void EventProviderRegistered(LPCGUID providerId, PREGHANDLE regHandle) const;
        void EventProviderUnregistered(REGHANDLE regHandle) const;
        void InvokingUserFunction(LPCGUID providerId, USHORT eventId) const;
        ULONG UserWriteString(UCHAR Level, ULONGLONG Keyword, PCWSTR String) const;
        
    private:
        REGHANDLE regHandle_;
        EventWriteFunc eventWrite_;

        template <typename T>
        static void AddUserData(std::vector<EVENT_DATA_DESCRIPTOR> & userData, T const & data)
        {
            EVENT_DATA_DESCRIPTOR item;
            EventDataDescCreate(&item, &data, sizeof(data));
            userData.push_back(item);
        }

        void WriteEvent(USHORT id, UCHAR level) const;
        void WriteEvent(USHORT id, UCHAR level, std::vector<EVENT_DATA_DESCRIPTOR> & userData) const;
        void WriteCommandEvent(USHORT id, UCHAR level, TracePointCommand::Data const & command) const;

        template <typename T1>
        void WriteEvent(USHORT id, UCHAR level, T1 const & data1) const
        {
            std::vector<EVENT_DATA_DESCRIPTOR> userData;
            AddUserData(userData, data1);
            WriteEvent(id, level, userData);
        }

        template <typename T1, typename T2>
        void WriteEvent(USHORT id, UCHAR level, T1 const & data1, T2 const & data2) const
        {
            std::vector<EVENT_DATA_DESCRIPTOR> userData;
            userData.reserve(2);
            AddUserData(userData, data1);
            AddUserData(userData, data2);
            WriteEvent(id, level, userData);
        }
    };
}
