// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if !defined(PLATFORM_UNIX)
#include "Store/SqlStore.h"
#endif

using namespace FederationTest;
using namespace std;
using namespace Federation;
using namespace Common;
using namespace Store;
using namespace Transport;
using namespace TestCommon;
using namespace FederationTestCommon;

wstring const FederationTestDispatcher::AddCommand = L"+";
wstring const FederationTestDispatcher::RemoveCommand = L"-";
wstring const FederationTestDispatcher::ConsiderCommand = L"consider";
wstring const FederationTestDispatcher::ExtendHoodCommand = L"extendhood";
wstring const FederationTestDispatcher::ShowCommand = L"show";
wstring const FederationTestDispatcher::SetPropertyCommand = L"set";
wstring const FederationTestDispatcher::SendOneWayCommand = L"sendone";
wstring const FederationTestDispatcher::SendRequestCommand = L"sendreq";
wstring const FederationTestDispatcher::RouteOneWayCommand = L"routeone";
wstring const FederationTestDispatcher::RouteRequestCommand = L"routereq";
wstring const FederationTestDispatcher::BroadcastOneWayCommand = L"broadcastone";
wstring const FederationTestDispatcher::BroadcastOneWayReliableCommand = L"broadcastreliable";
wstring const FederationTestDispatcher::BroadcastRequestCommand = L"broadcastreq";
wstring const FederationTestDispatcher::MulticastCommand = L"multicast";
wstring const FederationTestDispatcher::VerifyCommand = L"verify";
wstring const FederationTestDispatcher::ListCommand = L"list";
wstring const FederationTestDispatcher::AbortCommand = L"abort";
wstring const FederationTestDispatcher::LeaseCommand = L"lease";
wstring const FederationTestDispatcher::ClearTicketCommand = L"clearticket";
wstring const FederationTestDispatcher::FailLeaseAgentCommand = L"failleaseagent";
wstring const FederationTestDispatcher::BlockLeaseAgentCommand = L"blockleaseagent";
wstring const FederationTestDispatcher::BlockLeaseConnectionCommand = L"blockleaseconnection";
wstring const FederationTestDispatcher::UnblockLeaseConnectionCommand = L"unblockleaseconnection";
wstring const FederationTestDispatcher::AddLeaseBehaviorCommand = L"addleasebehavior";
wstring const FederationTestDispatcher::RemoveLeaseBehaviorCommand = L"removeleasebehavior";
wstring const FederationTestDispatcher::TicketStoreCommand = L"ticketstore";
wstring const FederationTestDispatcher::CompactCommand = L"compact";
wstring const FederationTestDispatcher::ArbitratorCommand = L"arbitrator";
wstring const FederationTestDispatcher::RejectCommand = L"reject";
wstring const FederationTestDispatcher::ChangeRingCommand = L"cr";
wstring const FederationTestDispatcher::ReadVoterStoreCommand = L"readvoter";
wstring const FederationTestDispatcher::WriteVoterStoreCommand = L"writevoter";
wstring const FederationTestDispatcher::ArbitrationVerifierCommand = L"arbitration";

wstring const FederationTestDispatcher::ParamDelimiter = L" ";
wstring const FederationTestDispatcher::nodeDelimiter = L"|";
wstring const FederationTestDispatcher::idInstanceDelimiter = L":";

wstring const GlobalBehavior = L"*";
wstring const MaxTimeValue = L"Max";

wstring const FederationTestDispatcher::StrictVerificationPropertyName = L"StrictVerification";
wstring const FederationTestDispatcher::UseTokenRangeForExpectedRoutingPropertyName = L"UseTokenRangeForExpectedRouting";
wstring const FederationTestDispatcher::ExpectFailureParameter = L"expectfailure";
wstring const FederationTestDispatcher::ExpectJoinErrorParameter = L"expectjoinerror";
wstring const FederationTestDispatcher::VersionParameter = L"version";
wstring const FederationTestDispatcher::PortParameter = L"port";
wstring const FederationTestDispatcher::CheckForLeakPropertyName = L"CheckForLeak";
wstring const FederationTestDispatcher::TestNodeRetryOpenPropertyName = L"RetryOpen";

wstring const FederationTestDispatcher::CredentialTypePropertyName = L"CredentialType";

wstring const FederationTestDispatcher::StringSuccess = L"Success";

wstring FederationTestDispatcher::TestDataDirectory = L".\\FederationTestData";

SecurityProvider::Enum FederationTestDispatcher::SecurityProvider = SecurityProvider::None;

bool FederationTestDispatcher::DefaultVerifyVoterStore = true;
bool FederationTestDispatcher::DefaultVerifyGlobalTime = false;

wstring const StringTrue = L"true";
wstring const StringFalse = L"false";
wstring const StringRouteExact = L"routeExact";
wstring const StringDefault = L"default";
wstring const StringExpectPrefix = L"expect";

StringLiteral const TraceSource = "Output";

class SqlStoreFactory : public IStoreFactory
{
public:
    ErrorCode CreateLocalStore(StoreFactoryParameters const & parameters, __out ILocalStoreSPtr & store)
    {
        ServiceModel::HealthReport unused;
        return this->CreateLocalStore(parameters, store, unused);
    }

    ErrorCode CreateLocalStore(StoreFactoryParameters const & parameters, __out ILocalStoreSPtr &, __out ServiceModel::HealthReport &)
    {
        /*
        if (parameters.Type == StoreType::Sql)
        {
            store = make_shared<SqlStore>(parameters.ConnectionString);
            return ErrorCode::Success();
        };
        */
        Assert::CodingError("Store {0} not supported", parameters.ConnectionString);
    }
};

FederationTestDispatcher::FederationTestDispatcher()
    : useStrictVerification_(false),
    useTokenRangeForExpectedRouting_(false),
    retryOpen_(false),
    checkForLeak_(false)
{
    testFederation_ = nullptr;
}

void FederationTestDispatcher::InitializeTestFederations()
{
    if (testFederation_)
    {
        return;
    }

    VoteConfig const & votes = FederationConfig::GetConfig().Votes;
    for (auto it = votes.begin(); it != votes.end(); ++it)
    {
        if (!GetTestFederation(it->RingName))
        {
            federations_.push_back(make_unique<TestFederation>(it->RingName));
        }
    }
    
    testFederation_ = federations_.begin()->get();
}

bool FederationTestDispatcher::Open()
{ 
    return true;
}

bool FederationTestDispatcher::ExecuteCommand(wstring command)
{
	if (CommonDispatcher::ExecuteCommand(command))
	{
		return true;
	}

    StringCollection baseParamCollection;
    if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::AddCommand))
    {
        wstring param = command.substr(FederationTestDispatcher::AddCommand.size());
        StringUtility::Split<wstring>(param, baseParamCollection, FederationTestDispatcher::ParamDelimiter);
        return AddNode(baseParamCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::RemoveCommand))
    {
        wstring param = command.substr(FederationTestDispatcher::RemoveCommand.size());
        return RemoveNode(param);
    }

    StringUtility::Split<wstring>(command, baseParamCollection, FederationTestDispatcher::ParamDelimiter);
    if (baseParamCollection.size() == 0)
    {
        return false;
    }

    baseParamCollection.erase(baseParamCollection.begin());
	StringCollection paramCollection = CompactParameters(baseParamCollection);

    if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ConsiderCommand))
    {
        return Consider(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ExtendHoodCommand))
    {
        return ExtendHood(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ShowCommand))
    {
        return Show(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::SetPropertyCommand))
    {
        return SetProperty(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::SendOneWayCommand))
    {
        return SendMessage(paramCollection, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::SendRequestCommand))
    {
        return SendMessage(paramCollection, false);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::RouteOneWayCommand))
    {
        return SendMessage(paramCollection, true, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::RouteRequestCommand))
    {
        return SendMessage(paramCollection, false, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::BroadcastOneWayCommand))
    {
        return this->BroadcastMessage(paramCollection, false, false);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::BroadcastOneWayReliableCommand))
    {
        return this->BroadcastMessage(paramCollection, false, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::BroadcastRequestCommand))
    {
        return this->BroadcastMessage(paramCollection, true, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::MulticastCommand))
    {
        return this->MulticastMessage(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::VerifyCommand))
    {
        return VerifyAll(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::AbortCommand))
    {
        if (paramCollection.size() != 1)
        {
            return false;
        }

        return AbortNode(paramCollection[0]);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ListCommand))
    {
        return ListNodes();
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::LeaseCommand))
    {
        return ListLeases();
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ClearTicketCommand))
    {
        return ClearTicket(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::TicketStoreCommand))
    {
        return SetTicketStore(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::FailLeaseAgentCommand))
    {
        return FailLeaseAgent(paramCollection[0]);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::BlockLeaseAgentCommand))
    {
        return BlockLeaseAgent(paramCollection[0]);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::BlockLeaseConnectionCommand))
    {
        return BlockLeaseConnection(paramCollection, true);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::UnblockLeaseConnectionCommand))
    {
        return BlockLeaseConnection(paramCollection, false);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::AddLeaseBehaviorCommand))
    {
        return AddLeaseBehavior(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::RemoveLeaseBehaviorCommand))
    {
        return RemoveLeaseBehavior(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::CompactCommand))
    {
        return Compact(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ArbitratorCommand))
    {
        return DumpArbitrator(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::RejectCommand))
    {
        return SetMessageHandlerRejectError(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ChangeRingCommand))
    {
        return ChangeRing(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ReadVoterStoreCommand))
    {
        return ReadVoterStore(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::WriteVoterStoreCommand))
    {
        return WriteVoterStore(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, FederationTestDispatcher::ArbitrationVerifierCommand))
    {
        return ArbitrationVerifier(paramCollection);
    }
    else if (Common::StringUtility::StartsWith(command, L"VerifyOption"))
    {
        ParseVerifyOption(paramCollection[0], DefaultVerifyVoterStore, DefaultVerifyGlobalTime);
        return true;
    }
    else
    {
        return false;
    }
}

bool FederationTestDispatcher::ReadVoterStore(StringCollection const & params)
{
    NodeId nodeId = ParseNodeId(params[0]);
    TestNodeSPtr testNode = testFederation_->GetNode(nodeId);
    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node not found: {0}", params[0]);
        return false;
    }

    int64 currentSequence;
    int64 currentValue = testFederation_->GetStoreEntry(params[1], currentSequence);

    int64 expectedValue = 0;
    if (params.size() > 2)
    {
        if (params[2] == L"auto")
        {
            expectedValue = currentValue;
        }
        else
        {
            if (!TryParseInt64(params[2], expectedValue))
            {
                expectedValue = 0;
            }
        }
    }

    int64 expectedSequence = -1;
    if (params.size() > 3)
    {
        if (params[3] == L"auto")
        {
            expectedSequence = currentSequence;
        }
        else
        {
            if (!TryParseInt64(params[3], expectedSequence))
            {
                expectedSequence = -1;
            }
        }
    }

    bool expectFailure = (params[params.size() - 1] == ExpectFailureParameter);

    testNode->ReadVoterStore(params[1], expectedValue, expectedSequence, expectFailure);

    return true;
}

bool FederationTestDispatcher::WriteVoterStore(StringCollection const & params)
{
    NodeId nodeId = ParseNodeId(params[0]);
    TestNodeSPtr testNode = testFederation_->GetNode(nodeId);
    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node not found: {0}", params[0]);
        return false;
    }

    int64 value;
    if (!TryParseInt64(params[2], value))
    {
        TestSession::WriteError(
            TraceSource,
            "Invalid value: {0}", params[2]);
        return false;
    }

    int64 currentSequence;
    int64 currentValue = testFederation_->GetStoreEntry(params[1], currentSequence);

    int64 checkSequence = -1;
    if (params.size() > 3)
    {
        if (params[3] == L"auto")
        {
            checkSequence = currentSequence;
        }
        else
        {
            if (!TryParseInt64(params[3], checkSequence))
            {
                checkSequence = -1;
            }
        }
    }

    bool isStale = (value < currentValue);
    bool allowConflict = false;
    bool expectFailure = false;

    for (size_t i = 3; i < params.size(); i++)
    {
        if (params[i] == L"allowConflict")
        {
            allowConflict = true;
        }
        else if (params[i] == ExpectFailureParameter)
        {
            expectFailure = true;
        }
    }

    testNode->WriteVoterStore(params[1], value, checkSequence, allowConflict, isStale, expectFailure);

    return true;
}

bool FederationTestDispatcher::ArbitrationVerifier(StringCollection const & params)
{
    if (params[0] == L"add")
    {
        if (params.size() < 3)
        {
            return false;
        }

        TestNodeSPtr node1 = testFederation_->GetNode(ParseLargeInteger(params[1]));
        TestNodeSPtr node2 = testFederation_->GetNode(ParseLargeInteger(params[2]));
        testFederation_->AddArbitration(node1->SiteNodePtr->Id, node2->SiteNodePtr->Id);
        return true;
    }
    else if (params[0] == L"verify")
    {
        return testFederation_->VerifyArbitration();
    }

    return false;
}

void FederationTestDispatcher::PrintHelp(std::wstring const & command)
{
	TestSession::WriteError(
		TraceSource,
		"Incorrect usage of command: {0}", command);
}

bool FederationTestDispatcher::AddNode(Common::StringCollection const & params)
{
    InitializeTestFederations();

    wstring nodeIdStr = params[0];
    wstring laPort = L"";
    FabricCodeVersion version = *FabricCodeVersion::Default;
    bool expectFailure = false;
    bool expectJoinError = false;
    wstring port = L"0";
    wstring fd = nodeIdStr;

    for (size_t i = 1; i < params.size(); i++)
    {
        if (params[i].compare(ExpectFailureParameter) == 0)
        {
            expectFailure = true;
        }
        else if (params[i].compare(ExpectJoinErrorParameter) == 0)
        {
            expectJoinError = true;
        }
        else if (StringUtility::StartsWithCaseInsensitive(params[i], VersionParameter + L"="))
        {
            if (!FabricCodeVersion::TryParse(params[i].substr(VersionParameter.length() + 1), version))
            {
                TestSession::WriteError(TraceSource, "Incorrect version provided to add node command.", params);
                return false;
            }
        }
        else if (StringUtility::StartsWithCaseInsensitive(params[i], PortParameter + L"="))
        {
            port = params[i].substr(PortParameter.length() + 1);
        }
        else if (StringUtility::StartsWithCaseInsensitive(params[i], wstring(L"fd=")))
        {
            fd = params[i].substr(3);;
        }
        else if (laPort.length() == 0)
        {
            laPort = params[i];
        }
        else
        {
            TestSession::WriteError(TraceSource, "Incorrect arguments provided to add node command.", params);
            return false;
        }
    }

    NodeId nodeId = ParseNodeId(nodeIdStr);

    if (testFederation_->ContainsNode(nodeId))
    {
        TestSession::WriteError(TraceSource, "SiteNode with the same Id: {0} already exists.", params);
        return false;
    }

    TestNodeSPtr testNode = make_shared<TestNode>(*this, *testFederation_, GetTestNodeConfig(testFederation_->RingName, nodeId, port, laPort), version, fd, make_shared<SqlStoreFactory>(), checkForLeak_, expectJoinError);

    testNode->Open(FederationTestConfig::GetConfig().OpenTimeout, retryOpen_, [nodeId, this, expectFailure](EventArgs const &)
    {
        bool result = testFederation_->ReportDownNode(nodeId);
        wstring msg = wformatString("Node {0} Failed", nodeId);
        FEDERATIONSESSION.Validate(msg, result);
        testFederation_->RemoveNode(nodeId);
    });
    testFederation_->AddNode(testNode);
    return true;
}

NodeConfig FederationTestDispatcher::GetTestNodeConfig(wstring const & ringName, NodeId nodeId, wstring port, wstring laPort)
{
    // Always access seed nodes through config since seed nodes can
    // be set from config file or through command line override
    //
    VoteConfig const & seedNodes = FederationConfig::GetConfig().Votes;
    auto it = seedNodes.find(nodeId, ringName);
    if (it != seedNodes.end())
    {
        ASSERT_IF(it->RingName != testFederation_->RingName,
            "Adding vote {0} for federation {1}",
            it->Id, testFederation_->RingName);

        wstring connectionString = it->ConnectionString;
        port = connectionString.substr(connectionString.find(':') + 1);
    }

    wstring workingDirectory = GetWorkingDirectory(ringName, nodeId.ToString());
    if (!Directory::Exists(workingDirectory))
    {
        Directory::Create(workingDirectory);
    }

    if (laPort.empty())
    {
        laPort = wformatString(TestPortHelper::GetNextLeasePort());
    }

    wstring addr = AddressHelper::BuildAddress(port);
    wstring laAddr = AddressHelper::BuildAddress(laPort);
    return NodeConfig(nodeId, addr, laAddr, workingDirectory, ringName);
}

bool FederationTestDispatcher::RemoveNode(wstring const & params)
{
    if (params == L"*")
    {
        auto votes = FederationConfig::GetConfig().Votes;
        vector<wstring> nonSeeds;
        vector<wstring> seeds;

        testFederation_->ForEachTestNode([&](TestNodeSPtr const& testNode)
        {
            NodeId id = testNode->SiteNodePtr->Id;
            auto it = votes.find(id, testFederation_->RingName);
            if (it == votes.end())
            {
                nonSeeds.push_back(id.ToString());
            }
            else
            {
                if (it->Type == Federation::Constants::SeedNodeVoteType)
                {
                    seeds.push_back(id.ToString());
                }
            }
        });

        for (auto iterator = nonSeeds.begin(); iterator != nonSeeds.end(); iterator++)
        {
            RemoveNode(*iterator);
        }

        for (auto iterator = seeds.begin(); iterator != seeds.end(); iterator++)
        {
            RemoveNode(*iterator);
        }
    }
    else
    {
        NodeId nodeId = ParseNodeId(params);
        TestNodeSPtr testNode = testFederation_->GetNode(nodeId);
        testFederation_->RemoveNode(nodeId);
        testNode->Close();
        TestSession::WriteInfo(TraceSource, "SiteNode with the Id: {0} succesfully closed.", params);
    }

    return true;
}

bool FederationTestDispatcher::AbortNode(wstring const & params)
{
    NodeId nodeId = ParseNodeId(params);
    TestNodeSPtr testNode = testFederation_->GetNode(nodeId);
    testFederation_->RemoveNode(nodeId);
    testNode->Abort();
    TestSession::WriteInfo(TraceSource, "SiteNode with the Id: {0} succesfully Aborted.", params);
    return true;
}

bool FederationTestDispatcher::Consider(StringCollection const & params)
{
    if (params.size() < 2)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to Consider");
        return false;
    }

    NodeId ownerNodeId = ParseNodeId(params[0]);
    NodeId nodeIdToConsider = ParseNodeId(params[1]);

    NodeInstance nodeInstance;
    NodePhase::Enum nodePhase;
    wstring addr;
    bool isInserting = false;

    if (params.size() == 2)
    {
        SiteNodeSPtr target = testFederation_->GetNode(nodeIdToConsider)->SiteNodePtr;
        nodeInstance = target->Instance;
        nodePhase = target->Phase;
        addr = target->Address;
    }
    else if (params.size() == 3)
    {
        if (params[2] == L"wronginstance")
        {
            SiteNodeSPtr target = testFederation_->GetNode(nodeIdToConsider)->SiteNodePtr;

            // add wrong instance id
            nodeInstance = NodeInstance(target->Instance.getId(), target->Instance.getInstanceId() + 1);

            nodePhase = target->Phase;
            addr = target->Address;
        }
        else
        {
            THROW(microsoft::MakeWindowsErrorCode(ERROR_INVALID_DATA), "3rd paramater must be 'wronginstance'");
        }
    }
    else
    {
        uint64 instance;
        StringUtility::TryFromWString<uint64>(params[2], instance);
        nodeInstance = NodeInstance(nodeIdToConsider, instance);
        nodePhase = ConvertWStringToNodePhase(params[3][0]);
        addr = L"addr_" + params[1];

        if (params.size() >= 5 && StringUtility::AreEqualCaseInsensitive(params[4], StringTrue))
        {
            isInserting = true;
        }

        addr = (params.size() >= 6 ? params[5] : L"addr_" + params[1]);
    }

    FederationPartnerNodeHeader partnerNodeHeader(nodeInstance, nodePhase, addr, L"", 0, RoutingToken(), Common::Uri(), false, L"", 1);

    TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);
    testNode->Consider(partnerNodeHeader, isInserting);
    return true;
}

bool FederationTestDispatcher::ExtendHood(StringCollection const & params)
{
    if (params.size() != 3 && params.size() != 4)
    {
        // 3 parameters if no shutdown nodes specified
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to ExtendHood");
        return false;
    }

    auto message = make_unique<Transport::Message>();

    NodeIdRange range(ParseLargeInteger(params[1]), ParseLargeInteger(params[2]));

    message->Headers.Add(FederationNeighborhoodRangeHeader(range));

    if (params.size() == 4)
    {
        // parsing the shutdown nodes
        StringCollection nodes;
        StringUtility::Split<wstring>(params[3], nodes, nodeDelimiter);

        for (wstring const & node : nodes)
        {
            StringCollection idInstance;
            StringUtility::Split<wstring>(node, idInstance, idInstanceDelimiter);

            if (idInstance.size() != 1 && idInstance.size() != 2)
            {
                TestSession::WriteError(TraceSource, "Cannot parse node id and instance.");
                return false;
            }

            NodeId nodeId(ParseLargeInteger(idInstance[0]));

            NodeInstance instance;
            if (idInstance.size() == 1)
            {
                TestNodeSPtr testNode = testFederation_->GetNode(nodeId);
                instance = testNode->SiteNodePtr->Instance;
            }
            else
            {
                uint64 value;
                StringUtility::TryFromWString<uint64>(idInstance[1], value);
                instance = NodeInstance(nodeId, value);
            }

            message->Headers.Add(FederationPartnerNodeHeader(instance, NodePhase::Shutdown, L"addr_" + params[0], L"", 0, RoutingToken(), Common::Uri(), false, L"", 1));
        }
    }

    NodeId ownerNodeId(ParseLargeInteger(params[0]));
    TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);

    testNode->SiteNodePtr->Table.ProcessNeighborHeaders(*message, NodeInstance(NodeId::MinNodeId, 0), L"", true);

    return true;
}

bool FederationTestDispatcher::ListNodes()
{
    wstring listText;
    StringWriter sw(listText);

    for (auto it = federations_.begin(); it != federations_.end(); ++it)
    {
        vector<NodeId> sortedList;
        (*it)->ForEachTestNode([&](TestNodeSPtr const& testNode)
        {
            sortedList.push_back(testNode->SiteNodePtr->Id);
        });

        std::sort(sortedList.begin(), sortedList.end());
        sw.Write("Federation {0} {1} nodes: ", (*it)->RingName, sortedList.size());
        if (sortedList.size() > 0)
        {
            sw.Write("[{0}]", sortedList[0]);
            for (size_t i = 1; i < sortedList.size(); ++i)
            {
                sw.Write(", [{0}]", sortedList[i]);            
            }
        }
        sw.WriteLine();
    }

    TestSession::WriteInfo(TraceSource, "{0}", listText);
    return true;
}

bool FederationTestDispatcher::ListLeases()
{
    wstring result = ::Dummy_DumpLeases();
    TestSession::WriteInfo(TraceSource, "{0}", result);

    return true;
}

bool FederationTestDispatcher::Show(StringCollection const & params)
{
    if (params.size() == 0)
    {
        testFederation_->ForEachTestNode([&](TestNodeSPtr const& testNode)
        {
            ShowNodeDetails(testNode);
        });
    }
    else if (params.size() == 1)
    {
        NodeId ownerNodeId = ParseNodeId(params[0]);
        TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);
        ShowNodeDetails(testNode);
    }

    return true;
}

void FederationTestDispatcher::ShowNodeDetails(TestNodeSPtr const & node)
{
    TestSession::WriteInfo(
        TraceSource,
        "Node {0}:\n"
        "Token: {1}, size: {2}/{3}\n"
        "Routing Table: {4:l}",
        node->SiteNodePtr->Id,
        node->SiteNodePtr->Token,
        node->SiteNodePtr->Table.UpNodes,
        node->SiteNodePtr->Table.Size,
        node->SiteNodePtr->Table);
    // TestTrace capped at 5 args
}

bool FederationTestDispatcher::DumpArbitrator(StringCollection const & params)
{
    if (params.size() == 1)
    {
        NodeId ownerNodeId = ParseNodeId(params[0]);
        TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);

        TestSession::WriteInfo(
            TraceSource,
            "{0:a}",
            testNode->SiteNodePtr->GetVoteManager());

        return true;
    }
    else
    {
        return false;
    }
}

bool FederationTestDispatcher::Compact(StringCollection const & params)
{
    if (params.size() == 0)
    {
        testFederation_->ForEachTestNode([&](TestNodeSPtr const& testNode)
        {
            CompactNode(testNode);
        });
    }
    else if (params.size() == 1)
    {
        NodeId ownerNodeId = ParseNodeId(params[0]);
        TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);
        CompactNode(testNode);
    }
    else
    {
        return false;
    }

    return true;
}

void FederationTestDispatcher::CompactNode(TestNodeSPtr const & node)
{
    node->SiteNodePtr->Table.TestCompact();
}

bool FederationTestDispatcher::SendMessage(StringCollection const & params, bool sendOneWay, bool isRouted)
{
    bool useExactRouting = false;
    bool expectReceived = true;
    bool idempotent = true;
    wstring expectedErrorString = FederationTestDispatcher::StringSuccess;

    for (size_t i = 2; i < params.size(); i++)
    {
        if (StringUtility::AreEqualCaseInsensitive(params[i], StringRouteExact))
        {
            useExactRouting = true;
        }
        else if (StringUtility::StartsWith<wstring>(params[i], StringExpectPrefix))
        {
            expectReceived = false;
            expectedErrorString = params[i].substr(StringExpectPrefix.size());
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[i], StringRouteExact))
        {
            useExactRouting = true;
        }
        else if (StringUtility::StartsWith<wstring>(params[i], StringExpectPrefix))
        {
            expectReceived = false;
            expectedErrorString = params[i].substr(StringExpectPrefix.size());
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[i], L"!idempotent"))
        {
            idempotent = false;
        }
    }

    ErrorCodeValue::Enum expectedError = ParseErrorCode(expectedErrorString);

    uint64 targetInstance = 0;
    wstring target = params[1];
    wstring ringName = testFederation_->RingName;
    size_t index = target.find_first_of(L'@');
    if (index != wstring::npos)
    {
        ringName = target.substr(index + 1);
        target = target.substr(0, index);
    }

    index = target.find_first_of(L':');
    if (index != wstring::npos)
    {
        wstring instanceString = target.substr(index + 1);
        target = target.substr(0, index);
        if (instanceString != L"*")
        {
            StringUtility::TryFromWString(instanceString, targetInstance);
        }
        else
        {
            TestNodeSPtr targetNode = testFederation_->GetNode(ParseLargeInteger(target));
            if (!targetNode)
            {
                TestSession::WriteError(TraceSource, "Target node {0} not found", target);
                return false;
            }

            targetInstance = targetNode->SiteNodePtr->Instance.InstanceId;
        }
    }

    NodeId nodeIdFrom(ParseLargeInteger(params[0]));
    NodeId nodeIdTo(ParseLargeInteger(target));
    TimeSpan routingTimeout = (testFederation_->RingName == ringName ? FederationTestConfig::GetConfig().RouteTimeout : FederationTestConfig::GetConfig().CrossRingRouteTimeout);

    TestNodeSPtr testNode = testFederation_->GetNode(nodeIdFrom);
    if (sendOneWay)
    {
        if (isRouted)
        {
            NodeId closestSiteNode = CalculateClosestNodeTo(nodeIdTo, ringName, useTokenRangeForExpectedRouting_);
            testNode->RouteOneWayMessage(nodeIdTo, targetInstance, ringName, closestSiteNode, routingTimeout, FederationTestConfig::GetConfig().RouteRetryTimeout, expectReceived, useExactRouting, idempotent, expectedError);
        }
        else
        {
            testNode->SendOneWayMessage(nodeIdTo, ringName);
        }
    }
    else
    {
        if (isRouted)
        {
            NodeId closestSiteNode = CalculateClosestNodeTo(nodeIdTo, ringName, useTokenRangeForExpectedRouting_);
            testNode->RouteRequestReplyMessage(nodeIdTo, targetInstance, ringName, closestSiteNode, routingTimeout, FederationTestConfig::GetConfig().RouteRetryTimeout, expectReceived, useExactRouting, idempotent, expectedError);
        }
        else
        {
            testNode->SendRequestReplyMessage(nodeIdTo, FederationTestConfig::GetConfig().SendTimeout, ringName);
        }
    }

    return true;
}

bool FederationTestDispatcher::BroadcastMessage(Common::StringCollection const & params, bool isRequest, bool isReliable)
{
    if (params.size() < 1)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to BroadcastMessage");
        return false;
    }

    NodeId nodeIdFrom(ParseLargeInteger(params[0]));

    TestNodeSPtr testNode = testFederation_->GetNode(nodeIdFrom);

    bool toAllRings = false;
    set<wstring> nodeIds;
    set<NodeId> nodes;
    if (params.size() < 2)
    {
        testFederation_->GetNodeList(nodes);
        for (NodeId node : nodes)
        {
            nodeIds.insert(testFederation_->RingName.empty() ? node.ToString() :  node.ToString() + L"@" + testFederation_->RingName);
        }
    }
    else
    {
        wstring temp = params[1];
        if (StringUtility::StartsWith<wstring>(temp, L"*"))
        {
            toAllRings = true;

            for (auto it = federations_.begin(); it != federations_.end(); ++it)
            {
                nodes.clear();
                (*it)->GetNodeList(nodes);
                for (NodeId node : nodes)
                {
                    nodeIds.insert((*it)->RingName.empty() ? node.ToString() :  node.ToString() + L"@" + (*it)->RingName);
                }
            }

            temp = temp.substr(1);
        }

        vector<wstring> excludeNodes;
        StringUtility::Split<wstring>(temp, excludeNodes, L",");

        for (wstring const & exclude : excludeNodes)
        {
            NodeId id;
            if (NodeId::TryParse(exclude, id))
            {
                nodes.erase(id);
            }
        }
    }

    if (isRequest)
    {
        testNode->BroadcastRequest(nodeIds, FederationTestConfig::GetConfig().BroadcastReplyTimeout);
    }
    else
    {
        testNode->Broadcast(nodeIds, isReliable, toAllRings);
    }

    return true;
}

bool FederationTestDispatcher::MulticastMessage(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to BroadcastMessage");
        return false;
    }

    NodeId nodeIdFrom(ParseLargeInteger(params[0]));

    TestNodeSPtr testNode = testFederation_->GetNode(nodeIdFrom);

    vector<wstring> targets;
    StringUtility::Split<wstring>(params[1], targets, L",");

    vector<NodeInstance> nodes;
    vector<NodeInstance> expectedFailedTargets;
    vector<NodeInstance> pendingTargets;
    for (wstring const & target : targets)
    {
        NodeId id;
        if (!NodeId::TryParse(target, id))
        {
            TestSession::WriteError(TraceSource, "Invalid node id {0}", target);
            return false;
        }

        TestNodeSPtr targetNode;
        if (testFederation_->TryGetNode(id, targetNode))
        {
            nodes.push_back(targetNode->SiteNodePtr->Instance);
        }
        else
        {
            NodeInstance instance(id, 100);
            nodes.push_back(instance);
            expectedFailedTargets.push_back(instance);
        }
    }

    CommandLineParser parser(params, 2);

    TimeSpan timeout = TimeSpan::MaxValue;
    int timeoutInSeconds;
    if (parser.TryGetInt(L"timeout", timeoutInSeconds))
    {
        timeout = TimeSpan::FromSeconds(timeoutInSeconds);
    }

    wstring pendingTargetsString;
    if (parser.TryGetString(L"pending", pendingTargetsString))
    {
        targets.clear();
        StringUtility::Split<wstring>(pendingTargetsString, targets, L",");
        for (wstring const & target : targets)
        {
            NodeId id;
            if (!NodeId::TryParse(target, id))
            {
                TestSession::WriteError(TraceSource, "Invalid node id {0}", target);
                return false;
            }

            TestNodeSPtr targetNode;
            if (testFederation_->TryGetNode(id, targetNode))
            {
                pendingTargets.push_back(targetNode->SiteNodePtr->Instance);
            }
            else
            {
                TestSession::WriteError(TraceSource, "Node {0} not found", target);
                return false;
            }
        }
    }

    sort(expectedFailedTargets.begin(), expectedFailedTargets.end());
    sort(pendingTargets.begin(), pendingTargets.end());
    testNode->Multicast(nodes, expectedFailedTargets, pendingTargets, FederationTestConfig::GetConfig().RouteRetryTimeout, timeout);

    return true;
}

bool FederationTestDispatcher::SetProperty(StringCollection const & params)
{
    if (params.size() > 0 && StringUtility::AreEqualCaseInsensitive(params[0], L"Defaults"))
    {
        useStrictVerification_ = false;
        useTokenRangeForExpectedRouting_ = false;
        checkForLeak_ = false;
        retryOpen_ = false;
        
        FederationConfig::Test_Reset();

        return true;
    }

    if (params.size() != 2)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to SetProperty");
        return false;
    }

    bool result = true;
    if (StringUtility::AreEqualCaseInsensitive(params[0], StrictVerificationPropertyName))
    {
        if (StringUtility::AreEqualCaseInsensitive(params[1], StringTrue))
        {
            useStrictVerification_ = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[1], StringFalse))
        {
            useStrictVerification_ = false;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Argument provided to StrictVerification ({0}) cannot be parsed as a bool", params[1]);
            result = false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], UseTokenRangeForExpectedRoutingPropertyName))
    {
        if (StringUtility::AreEqualCaseInsensitive(params[1], StringTrue))
        {
            useTokenRangeForExpectedRouting_ = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[1], StringFalse))
        {
            useTokenRangeForExpectedRouting_ = false;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Argument provided to UseTokenRangeForExpectedRouting ({0}) cannot be parsed as a bool", params[1]);
            result = false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], CredentialTypePropertyName))
    {
        if (testFederation_ && testFederation_->Count() > 0)
        {
            TestSession::WriteError(TraceSource, "Credential type cannot be changed when there are existing nodes");
            result = false;
        }
        else
        {
            auto error = Transport::SecurityProvider::FromCredentialType(params[1], FederationTestDispatcher::SecurityProvider);
            if (error.IsSuccess())
            {
                TestSession::WriteInfo(TraceSource, "Security provider set to {0}", FederationTestDispatcher::SecurityProvider);
                FederationTestDispatcher::SetClusterSpnIfNeeded();
            }
            else
            {
                TestSession::WriteError(TraceSource, "Invalid security credential type: {0}", params[1]);
                result = false;
            }

#ifdef PLATFORM_UNIX
            if (FederationTestDispatcher::SecurityProvider == Transport::SecurityProvider::Ssl)
            {
                extern Global<InstallTestCertInScope> testCert;
                if (!testCert)
                {
                    testCert = make_global<InstallTestCertInScope>();
                }
            }
#endif
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], TestNodeRetryOpenPropertyName))
    {
        if (StringUtility::AreEqualCaseInsensitive(params[1], StringTrue))
        {
            retryOpen_ = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[1], StringFalse))
        {
            retryOpen_ = false;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Argument provided to RetryOpen ({0}) cannot be parsed as a bool", params[1]);
            result = false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], CheckForLeakPropertyName))
    {
        if (StringUtility::AreEqualCaseInsensitive(params[1], StringTrue))
        {
            checkForLeak_ = true;
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[1], StringFalse))
        {
            checkForLeak_ = false;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Argument provided to CheckForLeak ({0}) cannot be parsed as a bool", params[1]);
            result = false;
        }
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationTestConfig::GetConfig().OpenTimeoutEntry.Key))
    {
        double seconds = Double_Parse(params[1]);
        FederationTestConfig::GetConfig().OpenTimeout = TimeSpan::FromSeconds(seconds);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationTestConfig::GetConfig().SendTimeoutEntry.Key))
    {
        double seconds = Double_Parse(params[1]);
        FederationTestConfig::GetConfig().SendTimeout = TimeSpan::FromSeconds(seconds);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationTestConfig::GetConfig().RouteTimeoutEntry.Key))
    {
        double seconds = Double_Parse(params[1]);
        FederationTestConfig::GetConfig().RouteTimeout = TimeSpan::FromSeconds(seconds);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationTestConfig::GetConfig().RouteRetryTimeoutEntry.Key))
    {
        double seconds = Double_Parse(params[1]);
        FederationTestConfig::GetConfig().RouteRetryTimeout = TimeSpan::FromSeconds(seconds);
    }
    else 
    {
        if (testFederation_)
        {
            TestSession::FailTestIfNot(testFederation_->Count() == 0, "Federation is not empty:{0}", testFederation_->Count());
        }

        if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().JoinLockDurationEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().JoinLockDuration = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().BootstrapTicketLeaseDurationEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().BootstrapTicketLeaseDuration = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().GlobalTicketLeaseDurationEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().GlobalTicketLeaseDuration = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().GlobalTicketRenewIntervalEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().GlobalTicketRenewInterval = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().VoteOwnershipLeaseIntervalEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().VoteOwnershipLeaseInterval = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().BootstrapTicketAcquireLimitEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().BootstrapTicketAcquireLimit = TimeSpan::FromSeconds(seconds);
        }

        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().PingIntervalEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().PingInterval = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().UpdateIntervalEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().UpdateInterval = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().LeaseDurationEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().LeaseDuration = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().NeighborhoodSizeEntry.Key))
        {
            int hoodSize = Int32_Parse(params[1]);
            FederationConfig::GetConfig().NeighborhoodSize = hoodSize;
            testFederation_->NeighborhoodSize = hoodSize;
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().TokenAcquireTimeoutEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().TokenAcquireTimeout = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().LeaseSuspendTimeoutEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().LeaseSuspendTimeout = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().ArbitrationTimeoutEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().ArbitrationTimeout = TimeSpan::FromSeconds(seconds);
        }
        else if (StringUtility::AreEqualCaseInsensitive(params[0], FederationConfig::GetConfig().ArbitrationRequestDelayEntry.Key))
        {
            double seconds = Double_Parse(params[1]);
            FederationConfig::GetConfig().ArbitrationRequestDelay = TimeSpan::FromSeconds(seconds);
        }
        else
        {
            TestSession::WriteError(TraceSource, "Cannot set unrecognized config parameter '{0}'", params[0]);
            result = false;
        }
    }

    return result;
}

bool FederationTestDispatcher::VerifyAll(StringCollection const & params)
{
    TimeSpan timeout = FederationTestConfig::GetConfig().VerifyTimeout;
    wstring ringName;
	wstring option;

    for (wstring const & param : params)
    {
        int64 seconds;
        if (param[0] == L'-')
		{
			option = param;
		}
        else if (TryParseInt64(param, seconds))
        {
            timeout = TimeSpan::FromSeconds(static_cast<double>(seconds));
        }
        else if (ringName.empty())
        {
            ringName = param;
        }
        else
        {
            TestSession::WriteError(TraceSource, "Invaid parameter {0}", param);
            return false;
        }
    }

    StopwatchTime stopwatchTime(Stopwatch::Now() + timeout);
    FEDERATIONSESSION.WaitForExpectationEquilibrium(timeout);

    bool result = true;
    for (auto it = federations_.begin(); it != federations_.end(); ++it)
    {
        if (ringName.empty() || (ringName == (*it)->RingName) || (ringName == L"." && it->get() == testFederation_))
        {
			if (!VerifyFederation(it->get(), stopwatchTime, option))
            {
                set<NodeId> nodeList;
                (*it)->GetNodeList(nodeList);

                vector<NodeId> tmp;
                for (NodeId nodeId : nodeList)
                {
                    tmp.push_back(nodeId);
                }
                sort(tmp.begin(), tmp.end());
                TestSession::WriteInfo(TraceSource, "{0} nodes: {1}", tmp.size(), tmp);

                result = false;
            }
        }
    }

	if (!result)
	{
		TestSession::WriteError(
			TraceSource,
			"Verification failed with a Timeout of {0} seconds.",
			timeout.TotalSeconds());
	}

    return result;
}

void FederationTestDispatcher::ParseVerifyOption(wstring const & option, bool & verifyVoterStore, bool & verifyGlobalTime)
{
    if (option.find(L's') != wstring::npos)
	{
		verifyVoterStore = false;
	}	
    else if (option.find(L'S') != wstring::npos)
	{
		verifyVoterStore = true;
	}	
	
    if (option.find(L't') != wstring::npos)
	{
		verifyGlobalTime = false;
	}
	else if (option.find(L'T') != wstring::npos)
    {
		verifyGlobalTime = true;
	}
}

bool FederationTestDispatcher::VerifyFederation(TestFederation* federation, StopwatchTime stopwatchTime, wstring const & option)
{
    bool negativeVerify = (option.find(L'n') != wstring::npos);
	bool verifyVoterStore = DefaultVerifyVoterStore;
	bool verifyGlobalTime = DefaultVerifyGlobalTime;

    ParseVerifyOption(option, verifyVoterStore, verifyGlobalTime);

    if (federation->VerifyAllRoutingTables(stopwatchTime.RemainingDuration()))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Routing table verification succeeded for federation {0}",
            federation->RingName);
    }
	else
    {
		if (negativeVerify)
		{
			return true;
		}

        TestSession::WriteError(
            TraceSource,
            "Routing table verification failed for federation {0}",
            federation->RingName);
        return false;
    }

    if (federation->VerifyAllTokens(stopwatchTime.RemainingDuration()))
    {
        TestSession::WriteInfo(
            TraceSource,
            "Token ownership verification succeeded for federation {0}",
            federation->RingName);
    }
	else
	{
		if (negativeVerify)
		{
			return true;
		}

		TestSession::WriteError(
            TraceSource,
            "Token ownership verification failed for federation {0}",
            federation->RingName);
        return false;
    }

    if (verifyVoterStore)
    {
        if (federation->VerifyVoterStore(stopwatchTime.RemainingDuration()))
        {
            TestSession::WriteInfo(
                TraceSource,
                "Voter store verification succeeded for federation {0}",
                federation->RingName);
        }
        else
        {
            if (negativeVerify)
            {
                return true;
            }

            TestSession::WriteError(
                TraceSource,
                "Voter store verification failed for federation {0}",
                federation->RingName);
            return false;
        }
    }

    if (verifyGlobalTime)
    {
        if (federation->VerifyGlobalTime(stopwatchTime.RemainingDuration()))
        {
            TestSession::WriteInfo(
                TraceSource,
                "Global time verification succeeded for federation {0}",
                federation->RingName);
        }
        else
        {
            if (negativeVerify)
            {
                return true;
            }

            TestSession::WriteError(
                TraceSource,
                "Global time verification failed for federation {0}",
                federation->RingName);
            return false;
        }
    }

	return !negativeVerify;
}

NodePhase::Enum FederationTestDispatcher::ConvertWStringToNodePhase(wchar_t nodePhase)
{
    if (nodePhase == L'B')
    {
        return NodePhase::Booting;
    }
    else if (nodePhase == L'J')
    {
        return NodePhase::Joining;
    }
    else if (nodePhase == L'I')
    {
        return NodePhase::Inserting;
    }
    else if (nodePhase == L'R')
    {
        return NodePhase::Routing;
    }
    else if (nodePhase == L'S')
    {
        return NodePhase::Shutdown;
    }
    else
    {
        THROW(microsoft::MakeWindowsErrorCode(ERROR_INVALID_DATA), "Cannot parse NodePhase.");
    }
}

bool FederationTestDispatcher::ClearTicket(StringCollection const & params)
{
    if (params.size() == 0)
    {
        for (auto seed : FederationConfig::GetConfig().Votes)
        {
            if (seed.Type == Federation::Constants::SeedNodeVoteType)
            {
                RemoveTicketFile(seed.RingName, seed.Id.ToString());
            }
            else 
            {
                StoreFactoryParameters storeParams;
                storeParams.Type = StoreType::Sql;
                storeParams.ConnectionString = seed.ConnectionString;

                ILocalStoreSPtr store;
                SqlStoreFactory().CreateLocalStore(storeParams, store);
                ErrorCode err = store->Open();
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                Store::IStoreBase::TransactionSPtr trans;

                err = store->CreateTransaction(trans);

                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"VoteOwnerData", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"SuperTicket", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"GlobalTicket", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = trans->Commit();
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Close();
                if (!err.IsSuccess())
                {
                    return false;
                }
            }
        }

        if (Directory::Exists(FederationTestDispatcher::TestDataDirectory))
        {
            auto error = Directory::Delete(FederationTestDispatcher::TestDataDirectory, true);
            TestSession::FailTestIfNot(error.IsSuccess(), "Cannot delete directory {0} due to error {1}", FederationTestDispatcher::TestDataDirectory, error);
        }
    }
    else
    {
        for (wstring const & id : params)
        {
            RemoveTicketFile(testFederation_->RingName, id);
        }
    }

    return true;
}

bool FederationTestDispatcher::SetTicketStore(StringCollection const & params)
{
    if (params.size() != 3)
    {
        TestSession::WriteError(TraceSource, "SetTicketStore requires 3 arguments: <node ID>,<global ticket>,<super ticket>");
        return false;
    }

    DateTime now = DateTime::Now();
    NodeId nodeId = ParseNodeId(params[0]);
    VoteOwnerState state;

    double globalTicketSeconds = Double_Parse(params[1]);
    state.GlobalTicket = now.AddWithMaxValueCheck(TimeSpan::FromSeconds(globalTicketSeconds)); 

    double superTicketSeconds = Double_Parse(params[2]);
    state.SuperTicket = now.AddWithMaxValueCheck(TimeSpan::FromSeconds(superTicketSeconds));

    wstring ticketPath(GetWorkingDirectory(testFederation_ ? testFederation_->RingName : L"", nodeId.ToString()));
    if (!Directory::Exists(ticketPath))
    {
        Directory::Create(ticketPath);
    }

    auto fileName = Path::Combine(ticketPath, nodeId.ToString() + L".tkt");

    File ticketFile;
    auto error = ticketFile.TryOpen(fileName, FileMode::Create, FileAccess::Write, FileShare::None);
    if (!error.IsSuccess())
    {
        TestSession::WriteError(TraceSource, "Failed to open ticket file {0}:{1}", fileName, error);
        return false;
    }
    
    auto global = state.getGlobalTicket();
    auto super = state.getSuperTicket();

    ticketFile.Write(&global, sizeof(DateTime));
    ticketFile.Write(&super, sizeof(DateTime));

    return true;
}

wstring FederationTestDispatcher::GetWorkingDirectory(wstring const & ringName, wstring const & id)
{
    return Path::Combine(FederationTestDispatcher::TestDataDirectory, ringName + id);
}

bool FederationTestDispatcher::FailLeaseAgent(wstring const & param)
{
    wstring laAddress = AddressHelper::BuildAddress(param);
    if (!LeaseWrapper::LeaseConfig::GetConfig().DebugLeaseDriverEnabled)
    {
        TRANSPORT_LISTEN_ENDPOINT ListenEndPoint;
        if (!LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, laAddress, ListenEndPoint))
        {
            return false;
        }

        ::FaultLeaseAgent(&ListenEndPoint);

    }
    else
    {
        ::Dummy_FaultLeaseAgent(laAddress.c_str());
    }

    return true;
}

bool FederationTestDispatcher::BlockLeaseAgent(wstring const & param)
{
    wstring laAddress = AddressHelper::BuildAddress(param);
    if (!LeaseWrapper::LeaseConfig::GetConfig().DebugLeaseDriverEnabled)
    {
        TRANSPORT_LISTEN_ENDPOINT ListenEndPoint;
        if (!LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, laAddress, ListenEndPoint))
        {
            return false;
        }

        ::BlockLeaseAgent(&ListenEndPoint);

    }
    else
    {
        ::Dummy_FaultLeaseAgent(laAddress.c_str());
    }

    return true;
}

bool FederationTestDispatcher::BlockLeaseConnection(StringCollection const & params, bool isBlocking)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to BlockLeaseConnection");
        return false;
    }

    wstring localAddress = AddressHelper::BuildAddress(params[0]);
    wstring remoteAddress = AddressHelper::BuildAddress(params[1]);

    if (!LeaseWrapper::LeaseConfig::GetConfig().DebugLeaseDriverEnabled)
    {
        TRANSPORT_LISTEN_ENDPOINT ListenEndPointLocal;
        TRANSPORT_LISTEN_ENDPOINT ListenEndPointRemote;

        if (!LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, localAddress, ListenEndPointLocal))
        {
            return false;
        }

        if (!LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, remoteAddress, ListenEndPointRemote))
        {
            return false;
        }

        ::BlockLeaseConnection(&ListenEndPointLocal, &ListenEndPointRemote, isBlocking);
    }

    return true;
}

bool FederationTestDispatcher::AddLeaseBehavior(StringCollection const & params)
{
    if (params.size() < 3 || params.size()>4)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to addLeaseBehaviorhavior");
        return false;
    }
    wstring localAddress = AddressHelper::BuildAddress(params[0]);
    wstring remoteAddress = AddressHelper::BuildAddress(params[1]);

    bool fromAny = (localAddress[localAddress.size() - 1] == L'*');
    bool toAny = (remoteAddress[remoteAddress.size() - 1] == L'*');
    // if true when implemented, the message is dropped at receiver end
    bool biDirection = false;
    wstring messageType = params[2];
    if (StringUtility::EndsWith<wstring>(messageType, L"~"))
    {
        biDirection = true;
        messageType = messageType.substr(0, messageType.size() - 1);
    }

    wstring alias;
    if (params.size() == 4){
        alias = params[3];
        if (biDirection)
        {
            biDirectionTransportCommands_.insert(alias);
        }
    }

    LEASE_BLOCKING_ACTION_TYPE messageTypeEnum;
    if (messageType == L"request"){
        messageTypeEnum = LEASE_ESTABLISH_ACTION;
    }
    else if (messageType == L"response"){
        messageTypeEnum = LEASE_ESTABLISH_RESPONSE;
    }
    else if (messageType == L"indirect"){
        messageTypeEnum = LEASE_INDIRECT;
    }
    else if (messageType == L"ping_response"){
        messageTypeEnum = LEASE_PING_RESPONSE;
    }
    else if (messageType == L"ping_request"){
        messageTypeEnum = LEASE_PING_REQUEST;
    }
    else{
        messageTypeEnum = LEASE_BLOCKING_ALL;
    }

    if (!LeaseWrapper::LeaseConfig::GetConfig().DebugLeaseDriverEnabled)
    {
        TRANSPORT_LISTEN_ENDPOINT ListenEndPointLocal = { 0 };
        TRANSPORT_LISTEN_ENDPOINT ListenEndPointRemote = { 0 };

        if (!fromAny && !LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, localAddress, ListenEndPointLocal))
        {
            return false;
        }

        if (!toAny && !LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, remoteAddress, ListenEndPointRemote))
        {
            return false;
        }

        ::addLeaseBehavior(&ListenEndPointRemote, toAny, &ListenEndPointLocal, fromAny, messageTypeEnum, biDirection ? alias + L"_1" : alias);
    }

    return true;
}

bool FederationTestDispatcher::RemoveLeaseBehavior(StringCollection const & params)
{
    if (params.size() > 1)
    {
        TestSession::WriteError(TraceSource, "Incorrect number of arguments provided to RemoveLeaseBehavior");
        return false;
    }

    wstring alias;
    if (params.size() == 1){
        alias = params[0];
    }

    if (biDirectionTransportCommands_.find(alias) != biDirectionTransportCommands_.end())
    {
        ::removeLeaseBehavior(alias + L"_1");
        ::removeLeaseBehavior(alias + L"_2");
        biDirectionTransportCommands_.erase(alias);
    }
    else
    {
        ::removeLeaseBehavior(alias);
    }
    
    return true;
}

void FederationTestDispatcher::RemoveTicketFile(wstring const & ringName, wstring const & id)
{
    wstring path = Path::Combine(GetWorkingDirectory(ringName, id), id + L".tkt");
    File::Delete(path, false);
}

wstring FederationTestDispatcher::GetState(std::wstring const & param)
{
    vector<wstring> params;
    StringUtility::Split<wstring>(param, params, L".");

	if (params.size() > 1)
	{
		NodeId id = ParseNodeId(params[params.size() - 1]);
		TestNodeSPtr testNode;
		if (testFederation_->TryGetNode(id, testNode))
		{
			params.erase(--params.end());
			return testNode->GetState(params);
		}
	}

    return wstring();
}

void FederationTestDispatcher::SetClusterSpnIfNeeded()
{
    if (!SecurityProvider::IsWindowsProvider(FederationTestDispatcher::SecurityProvider))
    {
        return;
    }

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    if (securityConfig.ClusterSpn.empty())
    {
        Transport::TransportSecurity transportSecurity;
        securityConfig.ClusterSpn = transportSecurity.LocalWindowsIdentity();
    }
}

bool FederationTestDispatcher::SetMessageHandlerRejectError(StringCollection const & params)
{
    if (params.size() < 1)
    {
        return false;
    }

    ErrorCode error;
    if (params.size() > 1)
    {
        if (params[1] == L"NotReady")
        {
            error = ErrorCodeValue::NotReady;
        }
        else
        {
            error = ErrorCodeValue::OperationFailed;
        }
    }

    NodeId ownerNodeId = ParseNodeId(params[0]);
    TestNodeSPtr testNode = testFederation_->GetNode(ownerNodeId);
    testNode->SetMessageHandlerRejectError(error);

    return true;
}

TestFederation* FederationTestDispatcher::GetTestFederation(std::wstring const & ringName)
{
    for (auto it = federations_.begin(); it != federations_.end(); ++it)
    {
        if ((*it)->RingName == ringName)
        {
            return it->get();
        }
    }
    
    return nullptr;
}

bool FederationTestDispatcher::ChangeRing(StringCollection const & params)
{
    if (params.size() == 0)
    {
        TestSession::WriteError(TraceSource, "Ring name must be supplied");
        return false;
    }

    InitializeTestFederations();

    auto result = GetTestFederation(params[0]);
    if (result)
    {
        testFederation_ = result;
    }
    else
    {
        federations_.push_back(make_unique<TestFederation>(params[0]));
        testFederation_ = federations_.rbegin()->get();
    }

    return true;
}

NodeId FederationTestDispatcher::CalculateClosestNodeTo(NodeId const & nodeId, wstring const & ringName, bool useTokenRangeForExpectedRouting)
{
    if (ringName.empty())
    {
        return testFederation_->CalculateClosestNodeTo(nodeId, useTokenRangeForExpectedRouting);
    }

    for (auto it = federations_.begin(); it != federations_.end(); ++it)
    {
        if ((*it)->RingName == ringName)
        {
            return (*it)->CalculateClosestNodeTo(nodeId, useTokenRangeForExpectedRouting); 
        }
    }

    Assert::CodingError("Ring {0} not found", ringName);
}
