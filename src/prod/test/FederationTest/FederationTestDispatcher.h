// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class FederationTestSession;

    class FederationTestDispatcher: public FederationTestCommon::CommonDispatcher
    {
        DENY_COPY(FederationTestDispatcher)
    public:
        FederationTestDispatcher();

        virtual bool Open();

        __declspec (property(get=getFederation)) TestFederation & Federation;

        bool ExecuteCommand(std::wstring command);

		void PrintHelp(std::wstring const & command);

        std::wstring GetState(std::wstring const & param);

        TestFederation & getFederation() { return *testFederation_; }

        TestFederation* GetFederation() { return testFederation_; }

        TestFederation* GetTestFederation(std::wstring const & ringName);

        static std::wstring const AddCommand;
        static std::wstring const RemoveCommand;
        static std::wstring const ConsiderCommand;
        static std::wstring const ExtendHoodCommand;
        static std::wstring const SendOneWayCommand;
        static std::wstring const SendRequestCommand;
        static std::wstring const RouteOneWayCommand;
        static std::wstring const RouteRequestCommand;
        static std::wstring const BroadcastOneWayCommand;
        static std::wstring const BroadcastOneWayReliableCommand;
        static std::wstring const BroadcastRequestCommand;
        static std::wstring const MulticastCommand;
        static std::wstring const VerifyCommand;
        static std::wstring const ShowCommand;
        static std::wstring const AbortCommand;
        static std::wstring const LeaseCommand;
        static std::wstring const ListCommand;
        static std::wstring const ParamDelimiter;
        static std::wstring const nodeDelimiter;
        static std::wstring const idInstanceDelimiter;
        static std::wstring const ClearTicketCommand;
        static std::wstring const TicketStoreCommand;
        static std::wstring const SetPropertyCommand;
        static std::wstring const FailLeaseAgentCommand;
        static std::wstring const BlockLeaseAgentCommand;
        static std::wstring const BlockLeaseConnectionCommand;
        static std::wstring const UnblockLeaseConnectionCommand;
        static std::wstring const AddLeaseBehaviorCommand;
        static std::wstring const RemoveLeaseBehaviorCommand;
        static std::wstring const CompactCommand;
        static std::wstring const ArbitratorCommand;
        static std::wstring const RejectCommand;
        static std::wstring const ChangeRingCommand;
        static std::wstring const ReadVoterStoreCommand;
        static std::wstring const WriteVoterStoreCommand;
        static std::wstring const ArbitrationVerifierCommand;

        static std::wstring TestDataDirectory;      

        static std::wstring const StrictVerificationPropertyName;
        static std::wstring const UseTokenRangeForExpectedRoutingPropertyName;
        static std::wstring const ExpectFailureParameter;
        static std::wstring const ExpectJoinErrorParameter;
        static std::wstring const VersionParameter;
        static std::wstring const PortParameter;
        static std::wstring const TestNodeRetryOpenPropertyName;
        static std::wstring const CheckForLeakPropertyName;

        static std::wstring const StringSuccess;

        static std::wstring const CredentialTypePropertyName;
        static Transport::SecurityProvider::Enum SecurityProvider;
        static void SetClusterSpnIfNeeded();
		
		static bool DefaultVerifyVoterStore;
		static bool DefaultVerifyGlobalTime;

    private:
        bool AddNode(Common::StringCollection const & params);
        bool RemoveNode(std::wstring const & params);
        bool AbortNode(std::wstring const & params);
        bool Consider(Common::StringCollection const & params);
        bool ExtendHood(Common::StringCollection const & params);
        bool Show(Common::StringCollection const & params);
        void ShowNodeDetails(TestNodeSPtr const & node);
        bool Compact(Common::StringCollection const & params);
        void CompactNode(TestNodeSPtr const & node);
        bool DumpArbitrator(Common::StringCollection const & params);
        bool SendMessage(Common::StringCollection const & params, bool sendOneWay, bool isRouted = false);
        bool BroadcastMessage(Common::StringCollection const & params, bool isRequest, bool isReliable);
        bool MulticastMessage(Common::StringCollection const & params);
        bool VerifyAll(Common::StringCollection const & params);
        bool VerifyFederation(TestFederation* federation, Common::StopwatchTime stopwatchTime, std::wstring const & option);
        bool SetProperty(Common::StringCollection const & params);
        bool ListNodes();
        bool ListLeases();
        bool ListBehaviors();
        bool FailLeaseAgent(std::wstring const & param);
        bool BlockLeaseAgent(std::wstring const & param);
        bool BlockLeaseConnection(Common::StringCollection const & params, bool isBlocking = true);
        bool ReadVoterStore(Common::StringCollection const & params);
        bool WriteVoterStore(Common::StringCollection const & params);
        bool ArbitrationVerifier(Common::StringCollection const & params);
        bool AddLeaseBehavior(Common::StringCollection const & params);
        bool RemoveLeaseBehavior(Common::StringCollection const & params);

        Federation::NodePhase::Enum ConvertWStringToNodePhase(wchar_t nodePhase);

        bool AddUnreliableTransportBehavior(Common::StringCollection const & params);
        bool RemoveUnreliableTransportBehavior(Common::StringCollection const & params);

        bool ClearTicket(Common::StringCollection const & params);

        bool SetTicketStore(Common::StringCollection const & params);

        void RemoveTicketFile(std::wstring const & ringName, std::wstring const & id);

        bool SetMessageHandlerRejectError(Common::StringCollection const & params);

        void InitializeTestFederations();

        bool ChangeRing(Common::StringCollection const & params);

        static std::wstring GetWorkingDirectory(std::wstring const & ringName, std::wstring const & id);
        
        Federation::NodeConfig GetTestNodeConfig(std::wstring const & ringName, Federation::NodeId nodeId, std::wstring port, std::wstring laPort = std::wstring());

        Federation::NodeId CalculateClosestNodeTo(Federation::NodeId const & nodeId, std::wstring const & ringName, bool useTokenRangeForExpectedRouting);

        static void ParseVerifyOption(std::wstring const & option, bool & verifyVoterStore, bool & verifyGlobalTime);
        
        TestFederation* testFederation_;
        std::vector<std::unique_ptr<TestFederation>> federations_;
        std::set<std::wstring> biDirectionTransportCommands_;

        bool useStrictVerification_;
        bool useTokenRangeForExpectedRouting_;
        bool retryOpen_;
        bool checkForLeak_;
    };
}
