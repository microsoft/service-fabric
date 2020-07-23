// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestDispatcher.h"
#include "TestSession.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ModelChecker;

wstring const TestDispatcher::LoadModelCommand = L"loadmodel";
wstring const TestDispatcher::VerifyCommand = L"verify";
wstring const TestDispatcher::TestStackCommand = L"TestStackCommand";
wstring const TestDispatcher::FMStateMachineTestCommand = L"TestFMStateMachine";
wstring const TestDispatcher::CheckModelCommand = L"checkmodel";

wstring const TestDispatcher::ParamDelimiter = L" ";
wstring const TestDispatcher::ItemDelimiter = L"|";
wstring const TestDispatcher::PairDelimiter = L"/";

wstring const TestDispatcher::EmptyCharacter = L"-";

StringLiteral const TestDispatcher::TraceSource = "ModelChecker";

TestDispatcher::TestDispatcher()
    : s0_(CreateInitialState()), stateSpaceExplorer_(s0_)
{
}

bool TestDispatcher::Bool_Parse(wstring const & input)
{
    if (StringUtility::AreEqualCaseInsensitive(input, L"true") ||
        StringUtility::AreEqualCaseInsensitive(input, L"yes") ||
        StringUtility::AreEqualCaseInsensitive(input, L"t") ||
        StringUtility::AreEqualCaseInsensitive(input, L"y"))
    {
        return true;
    }
    else if (
        StringUtility::AreEqualCaseInsensitive(input, L"false") ||
        StringUtility::AreEqualCaseInsensitive(input, L"no") ||
        StringUtility::AreEqualCaseInsensitive(input, L"f") ||
        StringUtility::AreEqualCaseInsensitive(input, L"n"))
    {
        return false;
    }
    else
    {
        Assert::CodingError("Value cannot be parsed into a boolean: {0}", input.c_str());
    }
}

StringCollection TestDispatcher::Split(wstring const& input, wstring const& delimiter)
{
    StringCollection results;
    StringUtility::Split(input, results, delimiter);
    return results;
}

StringCollection TestDispatcher::Split(wstring const& input, wstring const& delimiter, wstring const& emptyChar)
{
    // split the input and replace those "-" by empty
    StringCollection results;
    StringUtility::Split(input, results, delimiter);
    for (size_t i = 0; i < results.size(); ++i)
    {
        if (results[i] == emptyChar)
        {
            results[i].clear();
        }
    }

    return results;
}

bool TestDispatcher::Open()
{
    return true;
}

void TestDispatcher::Close()
{
	root_->FM->Close();
    TestCommon::TestDispatcher::Close();
}

bool TestDispatcher::ExecuteCommand(wstring command)
{
    StringCollection paramCollection = Split(command, ParamDelimiter, EmptyCharacter);
    if (paramCollection.size() == 0)
    {
        return false;
    }

    if (StringUtility::StartsWithCaseInsensitive(command, LoadModelCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return LoadModel(paramCollection);
    }
    else if (StringUtility::StartsWithCaseInsensitive(command, VerifyCommand))
    {
        paramCollection.erase(paramCollection.begin());
        return Verify(paramCollection);
    }
    else if (StringUtility::StartsWithCaseInsensitive(command, CheckModelCommand))
    {
        return CheckTheModel(paramCollection);
    }
    else
    {
        return false;
    }
}

wstring TestDispatcher::GetState(wstring const & param)
{
    param;
    return wstring();
}

bool TestDispatcher::LoadModel(Common::StringCollection const & params)
{
    if (params.size() != 1)
    {
        TestSession::WriteError(TraceSource, "Incorrect LoadModel parameters.");
        return false;
    }

    return true;
}

bool TestDispatcher::Verify(StringCollection const & params)
{
    if (params.size() > 0)
    {
        TestSession::WriteError(TraceSource, "Incorrect Verify parameters.");
        return false;
    }

    TimeSpan timeout = ModelCheckerConfig::GetConfig().VerifyTimeout;
    TimeoutHelper timeoutHelper(timeout);

    DWORD interval = 1000;

    bool ret = false;

    do
    {
        Sleep(interval);
    } while (!ret && !timeoutHelper.IsExpired);

    if (ret)
    {
        TestSession::WriteInfo(TraceSource, "All verification succeeded.");
    }
    else
    {
        TestSession::FailTest(
            "Verification failed with a Timeout of {0} seconds. Check logs for details.",
            timeout.TotalSeconds());
    }

    return true;
}

FailoverUnit::Flags TestDispatcher::FailoverUnitFlagsFromString(wstring const& value)
{
    // S:IsStateful, P:HasPersistedState, E:IsNoData, D:IsToBeDeleted
    bool isStateful = false;
    bool hasPersistedState = false;
    bool isNoData = false;
    bool isToBeDeleted = false;
    bool isSwappingPrimary = false;

    if (value != L"-")
    {
        set<wchar_t> charSet;
        for (size_t i = 0; i < value.size(); i++)
        {
            charSet.insert(value[i]);
        }

        auto itEnd = charSet.end();
        isStateful = (charSet.find(L'S') != itEnd);
        hasPersistedState = (charSet.find(L'P') != itEnd);
        isNoData = (charSet.find(L'E') != itEnd);
        isToBeDeleted = (charSet.find(L'D') != itEnd);
        isSwappingPrimary = (charSet.find(L'W') != itEnd);
    }

    return FailoverUnit::Flags(isStateful, hasPersistedState, isNoData, isToBeDeleted, isSwappingPrimary);
}

bool TestDispatcher::CheckTheModel(StringCollection const & params)
{
    if (params.size() != 2)
    {
        TestSession::WriteError(TraceSource, "Incorrect LoadModel parameters.");
        return false;
    }
    TestSession::WriteError(TraceSource, "DEPTH = {0}", Int32_Parse( params[1] ));
    stateSpaceExplorer_.BoundedDfs( Int32_Parse( params[1] ) );
    return true;
}

Epoch TestDispatcher::EpochFromString(wstring const& value)
{
    size_t len = value.size();
    int64 v0 = (len >= 1 ? value[len - 1] - L'0' : 0);
    int64 v1 = (len >= 2 ? value[len - 2] - L'0' : 0);
    int64 v2 = (len >= 3 ? value[len - 3] - L'0' : 0);

    return Epoch(v2, (v1 << 32) + v0);
}

FailoverUnitUPtr TestDispatcher::EmptyFailoverUnitFromString(wstring const& failoverUnitStr)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(failoverUnitStr, tokens, L" ");

    wstring const serviceName = L"TestService";
    int targetReplicaSetSize = Int32_Parse(tokens[0]);
    int minReplicaSetSize = Int32_Parse(tokens[1]);

    FailoverUnit::Flags flags = FailoverUnitFlagsFromString(tokens[2]);

    vector<wstring> versions;
    StringUtility::Split<wstring>(tokens[3], versions, L"/");

    Reliability::Epoch pcEpoch = EpochFromString(versions[0]);
    Reliability::Epoch ccEpoch = EpochFromString(versions[1]);

    ServiceModel::ApplicationIdentifier appId;
    ServiceModel::ApplicationIdentifier::FromString(L"TestApp_App0", appId);
    ApplicationInfoSPtr application = make_shared<ApplicationInfo>(appId, 1);
    wstring serviceTypeName = L"TestServiceType";
    ServiceModel::ServiceTypeIdentifier typeId(ServiceModel::ServicePackageIdentifier(appId, L"TestPackage"), serviceTypeName);
    wstring placementConstraints = L"";
    int scaleoutCount = 0;
    auto serviceType = make_shared<ServiceType>(typeId, application);

    auto serviceInfo = make_shared<ServiceInfo>(
            ServiceDescription(serviceName, 0, 0, 1, targetReplicaSetSize, minReplicaSetSize, flags.IsStateful(), flags.HasPersistedState(), TimeSpan::FromSeconds(60.0), TimeSpan::MaxValue, typeId, vector<ServiceCorrelationDescription>(), placementConstraints, scaleoutCount, vector<ServiceLoadMetricDescription>(), 0, vector<byte>()),
            serviceType,
			FABRIC_INVALID_SEQUENCE_NUMBER);

    FailoverUnitUPtr failoverUnit = FailoverUnitUPtr(new FailoverUnit(
            FailoverUnitId(),
            ConsistencyUnitDescription(),
            serviceInfo,
            flags,
            pcEpoch,
            ccEpoch,
            0,
            DateTime::Now(),
            0,
            0,
            PersistenceState::NoChange));

    failoverUnit->PostUpdate(DateTime::Now());

    return move(failoverUnit);

}

NodeId TestDispatcher::CreateNodeId(int id)
{
    return NodeId(LargeInteger(0, id));
}

NodeInstance TestDispatcher::CreateNodeInstance(int id, uint64 instance)
{
    return NodeInstance( CreateNodeId(id), instance );
}

void TestDispatcher::InitializeRoot()
{
    Federation::NodeConfig nodeConfig(Federation::NodeId(), L"TestAddress", L"TestAddress", L"");       
    root_ = Root::Create(nodeConfig);
    FailoverConfig & config = FailoverConfig::GetConfig();
    config.IsTestMode = true;
    config.FMStoreDirectory = L".\\FailoverUnitTestStoreDirectory";
    if (Directory::Exists(config.FMStoreDirectory))
    {
        auto error = Directory::Delete(config.FMStoreDirectory, true);
        ASSERT_IFNOT(error.IsSuccess(), "Unable to delete store directory: {0} due to error {1}", config.FMStoreDirectory, error);
    }
    Directory::Create(config.FMStoreDirectory);
    root_->FM->Open(EventHandler(), EventHandler());
}

State TestDispatcher::InitialState()
{
    // Initial state has an empty failover unit with three as the target size
    // there are five nodes, three of them are up
    // only timer event is enabled
    vector<NodeRecord> nodeList;
    for (int i = 0; i < 3; i++)
    {
        NodeInfo nodeInfo(
            CreateNodeInstance(i, i), 
            NodeDescription(), 
            true, // isup
            false, // isReplicaUploaded
            false, // isPendingShutdown 
            false, // isNodeStateRemoved 
            DateTime::Now() ); 
        NodeRecord nodeRecord( nodeInfo, false );
        nodeList.push_back(nodeRecord);
    }
    for (int i = 3; i < 5; i++)
    {
        NodeInfo nodeInfo( 
            CreateNodeInstance(i, i), 
            NodeDescription(), 
            false, 
            false, 
            false, 
            false, 
            DateTime::Now()); 
        NodeRecord nodeRecord( nodeInfo, false );
        nodeList.push_back(nodeRecord);
    }
    FauxCache fauxCache( nodeList );

    statelessTasks_.push_back(make_unique<StateUpdateTask>(*root_->FM, fauxCache));
    statelessTasks_.push_back(make_unique<StatelessCheckTask>(*root_->FM, fauxCache));
    statelessTasks_.push_back(make_unique<PendingTask>(*root_->FM));

    FailoverManagerComponent::FailoverUnitUPtr fmft = EmptyFailoverUnitFromString(L"3 0 - 000/111");

    Event teTimer( Event::Type::Timer, L"" );
    Event teNodeUp( Event::Type::NodeUp, L"" );
    Event teNodeDown( Event::Type::NodeDown, L"" );

    vector<Event> enabledEvents;
    enabledEvents.push_back(teTimer);
    enabledEvents.push_back(teNodeUp);
    enabledEvents.push_back(teNodeDown);

    return State( root_, statelessTasks_, move(fmft), move( fauxCache ), move(enabledEvents) );
}

State TestDispatcher::CreateInitialState()
{
    InitializeRoot();
    return InitialState();
}
