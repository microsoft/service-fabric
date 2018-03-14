// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class ClientTestSession : public TestSession
    {
        DENY_COPY(ClientTestSession)
    public:
        ClientTestSession(
            std::wstring const& serverAddress, 
            std::wstring const& clientId,
            std::wstring const& label, 
            bool autoMode, 
            TestDispatcher& dispatcher, 
            std::wstring const& dispatcherHelpFileName);

        void ReportClientData(std::wstring const& dataType, std::vector<byte> && data);

        __declspec(property(get=get_Runtime)) std::wstring const & ClientId;
        inline std::wstring const & get_Runtime() const { return clientId_; }

    protected:
        virtual std::wstring GetInput();

        virtual bool OnOpenSession();
        virtual void OnCloseSession();

    private:
        std::unique_ptr<Transport::IpcClient> ipcClient_;
        std::wstring clientId_;
        std::shared_ptr<Common::ReaderQueue<std::wstring>> commands_;
        Common::atomic_bool isClosed_;

        void ProcessServerMessage(Transport::MessageUPtr &, Transport::IpcReceiverContextUPtr & context);

        static int const ConnectTimeoutInSeconds;
    };
};
