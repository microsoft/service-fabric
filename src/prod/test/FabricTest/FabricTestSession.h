// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
#define FABRICSESSION  \
    FabricTestSession::GetFabricTestSession()    \

    class FabricTestSession;
    typedef std::shared_ptr<FabricTestSession> FabricTestSessionSPtr;    

    class FabricTestSession: public TestCommon::ServerTestSession
    {
        DENY_COPY(FabricTestSession)

    public:
        static void CreateSingleton(std::wstring const& label, bool autoMode, std::wstring const& serverListenAddress = FederationTestCommon::AddressHelper::GetServerListenAddress());
        static FabricTestSession & GetFabricTestSession();

        void AddCommand(std::wstring const & command);

        __declspec (property(get=get_FabricDispatcher)) FabricTestDispatcher & FabricDispatcher;
        FabricTestDispatcher & get_FabricDispatcher() {return fabricDispatcher_;}

        __declspec (property(get=get_AddressHelper)) FederationTestCommon::AddressHelper & AddressHelperObj;
        FederationTestCommon::AddressHelper & get_AddressHelper() {return addressHelper_;}

        std::shared_ptr<Common::AsyncOperation> BeginSendClientCommandRequest(
            std::wstring const& command,
            Transport::MessageId messageIdInReply,
            std::wstring const & clientId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndSendClientCommandRequest(Common::AsyncOperationSPtr const & operation, __out std::wstring & reply);

    protected:

        FabricTestSession(std::wstring const& label, bool autoMode, std::wstring const& serverListenAddress);

        virtual void OnClientDataReceived(std::wstring const& messageType, std::vector<byte> & data, std::wstring const& clientId);
        virtual void OnClientConnection(std::wstring const& clientId);
        virtual void OnClientFailure(std::wstring const& clientId);

        FabricTestDispatcher fabricDispatcher_;
        std::wstring section_;
        FederationTestCommon::AddressHelper addressHelper_;
        Transport::RequestTable requestTable_;

        static const Common::StringLiteral traceSource_;
        static const Common::StringLiteral FabricTestHelpFileName;

        static FabricTestSessionSPtr * singleton_;
    };
};
