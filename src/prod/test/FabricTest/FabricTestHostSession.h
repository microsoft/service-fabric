// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
#define FABRICHOSTSESSION  \
    FabricTestHostSession::GetFabricTestHostSession()    \

    class FabricTestHostSession;
    typedef std::shared_ptr<FabricTestHostSession> FabricTestHostSessionSPtr;

    class FabricTestHostSessionConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricTestHostSessionConfig, "FabricTestHost")
    };

    class FabricTestHostSession: public TestCommon::ClientTestSession
    {
        DENY_COPY(FabricTestHostSession)

    public:
        static void CreateSingleton(
            std::wstring const& nodeId,
            std::wstring const& label,
            std::wstring const& clientId, 
            bool autoMode,  
            std::wstring const& serverListenAddress = FederationTestCommon::AddressHelper::GetServerListenAddress());
        static FabricTestHostSession & GetFabricTestHostSession();

        void AddCommand(std::wstring const & command);

        __declspec (property(get=get_FabricTestHostDispatcher)) FabricTestHostDispatcher & Dispatcher;
        FabricTestHostDispatcher & get_FabricTestHostDispatcher() {return fabricDispatcher_;}

        __declspec (property(get=get_NodeId)) std::wstring const& NodeId;
        std::wstring const& get_NodeId() {return nodeId_;}

    private:
        FabricTestHostSession(
            std::wstring const& nodeId, 
            std::wstring const& label,
            std::wstring const& clientId, 
            bool autoMode, 
            std::wstring const& serverListenAddress);

        FabricTestHostDispatcher fabricDispatcher_;
        std::wstring const& nodeId_;

        static const Common::StringLiteral TraceSource;

        static FabricTestHostSessionSPtr * singleton_;
    };
};
