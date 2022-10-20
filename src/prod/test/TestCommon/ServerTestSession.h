// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class ServerTestSession : public TestSession
    {
        DENY_COPY(ServerTestSession)
    public:
        ServerTestSession(
            std::wstring const& listenAddress, 
            std::wstring const& label, 
            Common::TimeSpan maxIdleTime,
            bool autoMode, 
            TestDispatcher& dispatcher, 
            std::wstring const& dispatcherHelpFileName);

        void ExecuteCommandAtClient(std::wstring const& command, std::wstring const& clientId);
        void ExecuteCommandAtAllClients(std::wstring const& command);

    protected:
        virtual void OnClientDataReceived(std::wstring const& messageType, std::vector<byte> & data, std::wstring const& clientId) = 0;
        virtual void OnClientConnection(std::wstring const& clientId) = 0;
        virtual void OnClientFailure(std::wstring const& clientId) = 0;

        virtual bool OnOpenSession();
        virtual void OnCloseSession();

    private:

        Transport::IpcServerUPtr ipcServer_;
        Common::ExclusiveLock lock_;
        std::map<std::wstring, Common::ProcessWaitSPtr> clientMap_;
        bool isClosed_;
        Common::TimeSpan maxIdleTime_;

        void ProcessClientMessage(Transport::MessageUPtr &, Transport::IpcReceiverContextUPtr & context);
        bool StartMonitoringClient(DWORD processId, std::wstring const& clientId, __out bool & processExitedBeforeOpen);
        void OnClientProcessTerminate(DWORD processId, std::wstring const& clientId);
    };
};
