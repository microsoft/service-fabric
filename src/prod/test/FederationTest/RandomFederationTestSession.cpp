// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <psapi.h>

using namespace std;
using namespace FederationTest;
using namespace Federation;
using namespace Common;

int const RandomFederationTestSession::DefaultPingIntervalInSeconds = 3;

RandomFederationTestSession::RingContext::RingContext(RandomFederationTestSession & session, wstring const & ringName, vector<LargeInteger> const & allNodes, int baseLeaseAgentPort)
    :   session_(session), 
        ringName_(ringName),
        allNodes_(allNodes),
        baseLeaseAgentPort_(baseLeaseAgentPort), 
        belowQuorumStartTime_(StopwatchTime::Zero),
        lastArbitrationTime_(StopwatchTime::Zero)
{
    RandomFederationTestConfig const & config = session_.config_;
    seedCount_ = static_cast<size_t>(config.SeedNodeCount);
    quorum_ = (seedCount_ / 2) + 1;

    for (int i = 0; i < sizeof(storeValues_) / sizeof(int64); i++)
    {
        storeValues_[i] = 0;
    }
}

wstring RandomFederationTestSession::RingContext::GetSeedNodes()
{
    wstring result;
    StringWriter w(result);

    for (size_t i = 0; i < seedCount_; i++)
    {
        w.Write(" {0}", allNodes_[i]);
        if (session_.ringCount_ > 1)
        {
            w.Write("@{0}", ringName_);
        }

        liveNodes_.push_back(allNodes_[i]);
        liveSeedNodes_.insert(make_pair(allNodes_[i], Stopwatch::Now()));
    }

    sort(liveNodes_.begin(), liveNodes_.end());

    return result;
}

void RandomFederationTestSession::RingContext::AddSeedNodes()
{
    for (size_t i = 0; i < seedCount_; i++)
    {
        session_.AddInput(FederationTestDispatcher::AddCommand + allNodes_[i].ToString() + wformatString(" {0}", GetLeaseAgentPort(allNodes_[i])));
    }
}

bool RandomFederationTestSession::RingContext::ContainsNode(std::vector<Common::LargeInteger> const& nodes, Common::LargeInteger const& node)
{
    vector<LargeInteger>::const_iterator result = find(nodes.begin(), nodes.end(), node);
    bool res = result != nodes.end();
    return res;
}

bool RandomFederationTestSession::RingContext::IsSeedNode(LargeInteger const& node)
{
    for (size_t i = 0; i < seedCount_; i++)
    {
        if (allNodes_[i] == node)
        {
            return true;
        }
    }

    return false;
}

int RandomFederationTestSession::RingContext::GetLeaseAgentPort(LargeInteger const node) const
{
    for (size_t i = 0; i < allNodes_.size(); i++)
    {
        if (allNodes_[i] == node)
        {
            return baseLeaseAgentPort_ + static_cast<int>(i);
        }
    }

    Assert::CodingError("Node {0} not found", node);
}

int RandomFederationTestSession::RingContext::GetArbitratorCount()
{
    FederationConfig const & config = FederationConfig::GetConfig();
    StopwatchTime startTime = Stopwatch::Now() - (config.MaxLeaseDuration + config.ArbitrationTimeout + TimeSpan::FromSeconds(5));

    int count = 0;
    for (auto it = liveSeedNodes_.begin(); it != liveSeedNodes_.end(); ++it)
    {
        if (it->second < startTime)
        {
            count++;
        }
    }

    return count;
}

void RandomFederationTestSession::RingContext::InitializeFederationStage()
{
    nodeHistory_.clear();
}

LargeInteger RandomFederationTestSession::RingContext::SelectLiveNode()
{
    size_t r = static_cast<size_t>(session_.random_.Next()) % liveNodes_.size();
    return liveNodes_[r];
}

void RandomFederationTestSession::RingContext::AddNode(bool seedNodeOnly)
{
    LargeInteger node;
    size_t index = 0;

    do
    {
        size_t range = (seedNodeOnly ? seedCount_ : allNodes_.size());
        index = session_.random_.Next() % range;
        node = allNodes_[index];
    } while (ContainsNode(nodeHistory_, node) || ContainsNode(liveNodes_, node));

    liveNodes_.push_back(node);
    sort(liveNodes_.begin(), liveNodes_.end());

    if (IsSeedNode(node))
    {
        liveSeedNodes_.insert(make_pair(node, Stopwatch::Now()));
    }

    session_.AddInput(FederationTestDispatcher::AddCommand + node.ToString() + wformatString(" {0}", GetLeaseAgentPort(node)));

    nodeHistory_.push_back(node);
}

void RandomFederationTestSession::RingContext::RemoveNode(bool seedNodeOnly)
{
    size_t r = static_cast<size_t>(session_.random_.Next());

    bool done = false;
    while (!done)
    {
        r %= liveNodes_.size();
        LargeInteger id = liveNodes_[r];
        if (!ContainsNode(nodeHistory_, id) && (!seedNodeOnly || r < seedCount_))
        {
            int arbitratorCount = GetArbitratorCount();

            if (IsSeedNode(id))
            {
                if (seedNodeOnly || arbitratorCount > quorum_)
                {
                    auto it = liveSeedNodes_.find(id);
                    liveSeedNodes_.erase(it);
                    arbitratorCount = GetArbitratorCount();
                }
                else
                {
                    r++;
                    continue;
                }
            }

            liveNodes_.erase(liveNodes_.begin() + r);

            int randomSample = session_.random_.Next(100);
            if (randomSample < session_.config_.AbortRatio)
            {
                if ((randomSample % 3) > 0 && arbitratorCount >= quorum_ && !seedNodeOnly)
                {
                    if ((randomSample % 3) > 1)
                    {
                        int leaseAgentPort = GetLeaseAgentPort(id);

                        session_.AddInput(wformatString("!expect,Node {0} Failed", id));
                        session_.AddUnreliableTransportBehavior(wformatString("{0} * ArbitrateKeepAlive", id), true);
                        session_.AddUnreliableTransportBehavior(wformatString("{0} * ArbitrateRequest", id), true);
                        session_.AddInput(wformatString("addleasebehavior {0} * indirect", leaseAgentPort));
                        session_.AddInput(wformatString("blockleaseagent {0}", leaseAgentPort));
                    }
                    else
                    {
                        session_.AddInput(wformatString("blockleaseagent {0}", GetLeaseAgentPort(id)));
                        session_.AddInput(FederationTestDispatcher::AbortCommand + L" " + id.ToString());
                    }

                    lastArbitrationTime_ = Stopwatch::Now();
                }
                else
                {
                    session_.AddInput(FederationTestDispatcher::AbortCommand + L" " + id.ToString());
                }
            }
            else
            {
                session_.AddInput(FederationTestDispatcher::RemoveCommand + id.ToString());
            }

            nodeHistory_.push_back(id);

            done = true;
        }

        r++;
    }
}

void RandomFederationTestSession::RingContext::AddOrRemoveNode()
{
    if (liveSeedNodes_.size() < quorum_ && Stopwatch::Now() > belowQuorumStartTime_ + TimeSpan::FromSeconds(30))
    {
        session_.AddInput(wformatString("#Seed node count {0} below quorum, restoring quorum", liveSeedNodes_.size()));
        while (liveSeedNodes_.size() < quorum_)
        {
            AddNode(true);
        }

        return;
    }

    int ratio = 100 * (static_cast<int>(liveNodes_.size()) - session_.config_.MinNodes) / (session_.config_.MaxNodes - session_.config_.MinNodes);

    int r = session_.random_.Next() % 100;
    if ((r >= ratio || ratio == 0) && ratio != 100)
    {
        AddNode(false);
    }
    else
    {
        FederationConfig const & config = FederationConfig::GetConfig();
        StopwatchTime now = Stopwatch::Now();
        if (liveSeedNodes_.size() == quorum_ &&
            now > belowQuorumStartTime_ + TimeSpan::FromTicks(config.GlobalTicketLeaseDuration.Ticks * 4) &&
            now > lastArbitrationTime_ + config.MaxLeaseDuration + config.ArbitrationTimeout &&
            session_.random_.Next() % 3 == 0)
        {
            int remove = session_.random_.Next() % (quorum_ - 1) + 1;
            session_.AddInput(wformatString("#Closing {0} seed nodes", remove));
            for (int i = 0; i < remove; i++)
            {
                RemoveNode(true);
            }

            belowQuorumStartTime_ = now;
        }
        else
        {
            RemoveNode(false);
        }
    }
}

void RandomFederationTestSession::RingContext::ExecuteArbitrationStage()
{
    FederationConfig const & config = FederationConfig::GetConfig();

    int arbitratorCount = GetArbitratorCount();
    if (arbitratorCount < quorum_ || liveNodes_.size() < config.NeighborhoodSize * 2 + 1)
    {
        return;
    }

    vector<LargeInteger> nodes = liveNodes_;
    sort(nodes.begin(), nodes.end());

    set<LargeInteger> blocked;
    set<LargeInteger> blockedSeeds;

    int nodeCount = static_cast<int>(nodes.size());
    int count = session_.random_.Next(session_.config_.MaxBlockedLeaseAgent);
    if (count > nodeCount - liveSeedNodes_.size())
    {
        count = nodeCount - static_cast<int>(liveSeedNodes_.size());
    }

    if (count == 0)
    {
        return;
    }

    session_.AddInput(wformatString("#Blocking {0} nodes, arbitrator={1}", count, arbitratorCount));
    session_.AddInput(wformatString("addbehavior blockkeepalive * * ArbitrateKeepAlive {0}", session_.random_.NextDouble()));
    session_.AddInput(wformatString("addleasebehavior * * indirect blockindirect"));

    for (int i = 0; i < count && arbitratorCount >= quorum_; i++)
    {
        int j = session_.random_.Next(nodeCount);
        LargeInteger id = nodes[j];

        while ((blocked.find(id) != blocked.end()) ||
               (IsSeedNode(id) && arbitratorCount == quorum_ && blockedSeeds.find(id) == blockedSeeds.end()))
        {
            j = (j + 1) % nodeCount;
            id = nodes[j];
        }

        session_.AddInput(wformatString("#Blocking {0} port {1}", id, GetLeaseAgentPort(id)));

        blocked.insert(id);
        if (IsSeedNode(id))
        {
            if (blockedSeeds.find(id) == blockedSeeds.end())
            {
                arbitratorCount--;
                blockedSeeds.insert(id);
            }
        }

        vector<LargeInteger> neighbors;
        int k = (j + nodeCount - config.NeighborhoodSize) % nodeCount;
        for (int n = 0; n < config.NeighborhoodSize * 2 + 1; n++)
        {
            LargeInteger neighbor = nodes[(k + n) % nodeCount];
            if (blocked.find(neighbor) == blocked.end())
            {
                neighbors.push_back(neighbor);
            }
        }

        random_shuffle(neighbors.begin(), neighbors.end());
        int blockCount = session_.random_.Next(static_cast<int>(neighbors.size()));

        for (k = 0; k < blockCount; k++)
        {
            if (IsSeedNode(neighbors[k]) && blockedSeeds.find(neighbors[k]) == blockedSeeds.end())
            {
                if (arbitratorCount == quorum_)
                {
                    continue;
                }

                blockedSeeds.insert(neighbors[k]);
                arbitratorCount--;
            }

            session_.AddInput(wformatString("{0} add {1} {2}", FederationTestDispatcher::ArbitrationVerifierCommand, id, neighbors[k]));
            session_.AddInput(wformatString("{0} {1} {2} *~", FederationTestDispatcher::AddLeaseBehaviorCommand, GetLeaseAgentPort(id), GetLeaseAgentPort(neighbors[k])));
        }
    }

    session_.AddInput(wformatString("!pause,{0}", (config.LeaseDuration + config.ArbitrationTimeout).TotalSeconds()));
    session_.AddInput(L"removebehavior blockkeepalive");
    session_.AddCommand(L"removeleasebehavior");
    session_.AddInput(L"!pause,10");
    session_.AddInput(wformatString("{0} verify", FederationTestDispatcher::ArbitrationVerifierCommand));
    session_.AddInput(L"verify");
}

void RandomFederationTestSession::RingContext::TestRouting()
{
    LargeInteger from = SelectLiveNode();
    LargeInteger target = LargeInteger::RandomLargeInteger_();
    wstring toRing;
    if (session_.ringCount_ > 1)
    {
        int r = session_.random_.Next(session_.ringCount_);
        if (session_.rings_[r]->GetRingName() != ringName_)
        {
            toRing = L"@" + session_.rings_[r]->GetRingName();
        }
    }

    session_.AddInput(FederationTestDispatcher::RouteRequestCommand + L" " + from.ToString() + L" " + target.ToString() + toRing);
}

void RandomFederationTestSession::RingContext::TestBroadcast()
{
    LargeInteger from = SelectLiveNode();
    int r = session_.random_.Next(4);
    if (r < 2)
    {
        wstring destination;
        if (r == 1 && session_.ringCount_ > 1)
        {
            destination = L" *";
        }

        session_.AddInput(FederationTestDispatcher::BroadcastOneWayReliableCommand + L" " + from.ToString() + destination);
    }
    else if (r == 2)
    {
        session_.AddInput(FederationTestDispatcher::BroadcastRequestCommand + L" " + from.ToString());
    }
    else
    {
        set<NodeId> targets;

        double ratio = session_.random_.NextDouble();
        for (size_t j = 0; j < liveNodes_.size(); j++)
        {
            if (session_.random_.NextDouble() > ratio)
            {
                targets.insert(liveNodes_[j]);
            }
        }

        if (targets.size() == 0)
        {
            for (size_t j = 0; j < liveNodes_.size(); j++)
            {
                targets.insert(liveNodes_[j]);
            }
        }

        while (session_.random_.Next(2) == 0)
        {
            NodeId extraId = NodeId(LargeInteger::RandomLargeInteger_());
            targets.insert(extraId);
        }

        wstring temp;
        for (auto it = targets.begin(); it != targets.end(); ++it)
        {
            if (temp.size() > 0)
            {
                temp = temp + L",";
            }
            temp = temp + it->ToString();
        }

        session_.AddInput(FederationTestDispatcher::MulticastCommand + L" " + from.ToString() + L" " + temp);
    }
}

void RandomFederationTestSession::RingContext::TestVoterStore()
{
    map<int, int> entries;
    for (int i = 0; i < session_.random_.Next(1, session_.config_.MaxStoreWrites); i++)
    {
        int index = session_.random_.Next(sizeof(storeValues_) / sizeof(int64));
        if (entries.find(index) == entries.end())
        {
            entries[index] = 1;
        }
        else
        {
            entries[index]++;
        }
    }

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        int index = it->first;
        wstring key = wformatString("key{0}", index);
        int64 oldValue = storeValues_[index];

        for (int i = 0; i < it->second; i++)
        {
            LargeInteger from = SelectLiveNode();
            bool checked = (it->second == 1 || session_.random_.Next(2) == 0);
            int64 value = (checked ? ++storeValues_[index] : oldValue + it->second);

            wstring command = wformatString("{0} {1} {2} {3}",
                FederationTestDispatcher::WriteVoterStoreCommand,
                from,
                key,
                value);

            if (checked)
            {
                command = command + L" auto";
                if (it->second > 1)
                {
                    command = command + L" allowConflict";
                }
            }

            session_.AddInput(command);
        }

        storeValues_[index] = oldValue + it->second;
    }

    session_.AddInput(FederationTestDispatcher::VerifyCommand + L" " + session_.waitTime_);

    for (int i = 0; i < session_.random_.Next(1, session_.config_.MaxStoreWrites); i++)
    {
        int index = session_.random_.Next(sizeof(storeValues_) / sizeof(int64));

        wstring key = wformatString("key{0}", index);
        LargeInteger from = SelectLiveNode();

        wstring command = wformatString("{0} {1} {2} auto auto",
            FederationTestDispatcher::ReadVoterStoreCommand,
            from,
            key);
        session_.AddInput(command);
    }
}

void RandomFederationTestSession::RingContext::UpdateNodes(set<NodeId> const & nodes)
{
    for (auto it = liveNodes_.begin(); it != liveNodes_.end();)
    {
        if (nodes.find(NodeId(*it)) == nodes.end())
        {
            liveSeedNodes_.erase(*it);
            it = liveNodes_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RandomFederationTestSession::CreateSingleton(int iterations, int timeout, wstring const& label, bool autoMode, int ringCount)
{
    shared_ptr<RandomFederationTestSession> randomFederationTestSession = shared_ptr<RandomFederationTestSession>(new RandomFederationTestSession(iterations, timeout, label, autoMode, ringCount));
    singleton_ = new shared_ptr<FederationTestSession>(static_pointer_cast<FederationTestSession>(randomFederationTestSession));
}

RandomFederationTestSession::RandomFederationTestSession(int iterations, int timeout, std::wstring const& label, bool autoMode, int ringCount) 
    :     FederationTestSession(label, autoMode),
    config_(RandomFederationTestConfig::GetConfig()),
    testStage_(0), 
    initalizeStage_(0),
    iterations_(iterations), 
    timeout_(timeout <= 0 ? TimeSpan::MaxValue : TimeSpan::FromSeconds(timeout)),
    random_(),
    highMemoryStartTime_(StopwatchTime::Zero),
    ringCount_(ringCount),
    currentRing_(0)
{
    vector<LargeInteger> allNodes;
    for (int i = 0; i < max(config_.TotalNodes, config_.SeedNodeCount); i++)
    {
        LargeInteger id = LargeInteger::RandomLargeInteger_();
        if (find(allNodes.begin(), allNodes.end(), id) == allNodes.end())
        {
            allNodes.push_back(id);
        }
    }

    int baseLeaseAgentPort = 17000;
    for (int i = 0; i < ringCount_; i++)
    {
        wstring ringName = (ringCount_ > 1 ? wformatString("r{0}", i) : L"");
        rings_.push_back(make_unique<RingContext>(*this, ringName, allNodes, baseLeaseAgentPort));
        baseLeaseAgentPort += 100;
    }
}

wstring RandomFederationTestSession::GetInput()
{
    if (initalizeStage_ == 0)
    {
        //ClearTicket
        AddInput(FederationTestDispatcher::ClearTicketCommand);
        AddInput(FederationTestDispatcher::RemoveLeaseBehaviorCommand);

        //Set OpenTimeout
        int64 openTimeoutInSeconds = config_.OpenTimeout.TotalSeconds();
        wstring openTimeout = wformatString(openTimeoutInSeconds);
        AddInput(FederationTestDispatcher::SetPropertyCommand + L" " + 
            FederationTestConfig::GetConfig().OpenTimeoutEntry.Key + L" " + 
            openTimeout);

        //Set RetryOpen
        AddInput(FederationTestDispatcher::SetPropertyCommand + L" " + 
            FederationTestDispatcher::TestNodeRetryOpenPropertyName + L" true");

        //Set Route Retry Timeout
        AddInput(FederationTestDispatcher::SetPropertyCommand + L" " + 
            FederationTestConfig::GetConfig().RouteRetryTimeoutEntry.Key + L" 2");

        //Set Route Timeout
        int64 routeTimeoutInSeconds = config_.RouteTimeout.TotalSeconds();
        wstring routeTimeout = wformatString(routeTimeoutInSeconds);
        AddInput(FederationTestDispatcher::SetPropertyCommand + L" " + 
            FederationTestConfig::GetConfig().RouteTimeoutEntry.Key + L" " + 
            routeTimeout);

        waitTime_ = wformatString(config_.WaitTime.TotalSeconds());

        AddInput(L"!updatecfg Federation.VoterStoreBootstrapWaitInterval=30");
        //AddInput(L"!updatecfg Federation.ArbitrationTimeout=15");
        //AddInput(L"!updatecfg Federation.LeaseDuration=15");

        initalizeStage_ = 1;

        watch_.Start();
    }

    if(!CheckMemory())
    {
        AddInput(L"#Memory check failed.  Aborting...");
        AddInput(TestSession::AbortCommand);
        return wstring();
    }
    else
    {
        if (timeout_.IsExpired)
        {
            AddInput(L"#Time limit reached.  Exiting...");
            AddInput(TestSession::QuitCommand);
            return wstring();
        }

        switch(testStage_)
        {
        case 0:
            {
                wstring text;
                StringWriter w(text);
                w.Write("Iterations left: {0}, time elapsed: {1}", iterations_, TimeSpan::FromTicks(watch_.ElapsedTicks));
                TraceConsoleSink::Write(0x0b, text);
            }
            ExecuteFederationStage();
            AddInput(FederationTestDispatcher::VerifyCommand + L" " + waitTime_);
            ClearUnreliableTransportBehavior();
            AddInput(FederationTestDispatcher::ListCommand);
            testStage_++;
            break;
        case 1:
            if (config_.MaxBlockedLeaseAgent > 0)
            {
                ExecuteArbitrationStage();
                AddInput(FederationTestDispatcher::ListCommand);
            }
            testStage_++;
            break;
        case 2:
            for (auto it = rings_.begin(); it != rings_.end(); ++it)
            {
                set<NodeId> nodes;
                TestFederation* federation = FederationDispatcher.GetTestFederation((*it)->GetRingName());
                ASSERT_IF(federation == nullptr, "Ring {0} not found", (*it)->GetRingName());
                federation->GetNodeList(nodes);
                (*it)->UpdateNodes(nodes);
            }

            if(config_.MaxRoutes > 0)
            {
                ExecuteRoutingStage();
                AddInput(TestSession::WaitCommand + L"," + waitTime_); 
            }

            testStage_++;
            break;
        case 3:
            if(config_.MaxBroadcasts > 0)
            {
                ExecuteBroadcastStage();
                AddInput(TestSession::WaitCommand + L"," + waitTime_);
            }

            testStage_++;
            break;
        case 4:
            if (config_.MaxStoreWrites > 0)
            {
                ExecuteVoterStoreStage();
                AddInput(TestSession::WaitCommand + L"," + waitTime_);
            }

            testStage_++;
            break;
        default:
            iterations_--;
            if(iterations_ == 0)
            {
                watch_.Stop();
                AddInput(L"-*");
                AddInput(TestSession::QuitCommand);
                return wstring();
            }
            else
            {
                testStage_ = 0;
            }
            break;
        }

        if(testStage_ == 0)
        {
            return GetInput();
        }

        return wstring();
    }
}

bool RandomFederationTestSession::CheckMemory()
{
    //todo, memory check should be consistent with Transport/Throttle.cpp
    //on Linux, ProcessInfo::DataSize() is checked for throttling
    //on Windows, PagefileUsage is checked for throttling
    size_t MaxAllowedPrivateBytes = static_cast<size_t>(config_.MaxAllowedMemoryInMB) * 1024 * 1024;

#ifdef PLATFORM_UNIX
    auto pmi = ProcessInfo::GetSingleton();
    string summary;
    auto rss = pmi->GetRssAndSummary(summary);

    if(MaxAllowedPrivateBytes != 0 /*Memory check disabled if 0*/ && rss > MaxAllowedPrivateBytes)
    {
        TestSession::WriteError(
            "FederationTest.Memory",
            "Federation session: workingSet/rss exceeds limit {0}, {1}", 
            MaxAllowedPrivateBytes,
            summary);

        if (highMemoryStartTime_ == StopwatchTime::Zero)
        {
            highMemoryStartTime_ = Stopwatch::Now();
        }

        bool result = highMemoryStartTime_ + config_.MaxAllowedMemoryTimeout > Stopwatch::Now();
        ASSERT_IF(!result && config_.AssertOnMemoryCheckFailure, "Memory check failed");
        return result;
    }

    TestSession::WriteInfo("FederationTest.Memory", "Federation session: {0}", summary);
    highMemoryStartTime_ = StopwatchTime::Zero;
    return true;

#else
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, 
        GetCurrentProcessId());
    TestSession::FailTestIf(NULL == hProcess, "OpenProcess failed");
    TestSession::FailTestIf(FALSE == GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)), "GetProcessMemoryInfo failed");

    CloseHandle( hProcess );

    if(MaxAllowedPrivateBytes != 0 /*Memory check disabled if 0*/ && pmc.WorkingSetSize > MaxAllowedPrivateBytes)
    {
        TestSession::WriteError(
            "FederationTest.Memory",
            "Federation session WorkingSet {0} more than {1}. PeakWorkingSetSize {2} MB, PageFileUsage {3} MB, PeakPagefileUsage {4} MB",
            pmc.WorkingSetSize / (1024 * 1024), 
            config_.MaxAllowedMemoryInMB, 
            pmc.PeakWorkingSetSize / (1024 * 1024),
            pmc.PagefileUsage / (1024 * 1024),
            pmc.PeakPagefileUsage / (1024 * 1024));

        if (highMemoryStartTime_ == StopwatchTime::Zero)
        {
            highMemoryStartTime_ = Stopwatch::Now();
        }

        bool result = highMemoryStartTime_ + config_.MaxAllowedMemoryTimeout > Stopwatch::Now();
        ASSERT_IF(!result && config_.AssertOnMemoryCheckFailure, "Memory check failed");
        return result;
    }
    else
    {
        TestSession::WriteInfo(
            "FederationTest.Memory",
            "Federation session current memory = {0} MB. PeakWorkingSetSize {1} MB, PageFileUsage {2} MB, PeakPagefileUsage {3} MB",
            pmc.WorkingSetSize / (1024 * 1024), 
            pmc.PeakWorkingSetSize / (1024 * 1024),
            pmc.PagefileUsage / (1024 * 1024),
            pmc.PeakPagefileUsage / (1024 * 1024));
        highMemoryStartTime_ = StopwatchTime::Zero;

        return true;
    }
#endif
}

void RandomFederationTestSession::ChangeRing(size_t index)
{
    if (index != currentRing_)
    {
        currentRing_ = index;
        AddInput(wformatString("{0} {1}", FederationTestDispatcher::ChangeRingCommand, rings_[index]->GetRingName()));
    }
}

void RandomFederationTestSession::SelectRing()
{
    if (ringCount_ > 1)
    {
        int index = random_.Next(0, ringCount_);
        ChangeRing(index);
    }
}

void RandomFederationTestSession::InitializeSeedNodes()
{
    wstring voteCommand = L"votes";
    for (size_t i = 0; i < rings_.size(); i++)
    {
        voteCommand = voteCommand + rings_[i]->GetSeedNodes();
    }

    AddInput(voteCommand);
    AddInput(FederationTestDispatcher::ClearTicketCommand);

    for (size_t i = 0; i < rings_.size(); i++)
    {
        ChangeRing(i);
        rings_[i]->AddSeedNodes();
    }
}

void RandomFederationTestSession::ExecuteFederationStage()
{
    if (initalizeStage_ == 1)
    {
        InitializeSeedNodes();
        initalizeStage_ = 2;
    }

    for (auto it = rings_.begin(); it != rings_.end(); ++it)
    {
        (*it)->InitializeFederationStage();
    }

    int dynamism = random_.Next(1, config_.MaxDynamism + 1);
    for (int i = 0; i < dynamism; i++)
    {
        SelectRing();
        rings_[currentRing_]->AddOrRemoveNode();
    }
}

void RandomFederationTestSession::ExecuteArbitrationStage()
{
    rings_[currentRing_]->ExecuteArbitrationStage();
}

void RandomFederationTestSession::ExecuteRoutingStage()
{
    int routes = random_.Next(1, config_.MaxRoutes);
    for (int i = 0; i < routes; i++)
    {
        SelectRing();
        rings_[currentRing_]->TestRouting();
    }
}

void RandomFederationTestSession::ExecuteBroadcastStage()
{
    int broadcasts = random_.Next(1, config_.MaxBroadcasts);
    for (int i = 0; i < broadcasts; i++)
    {
        SelectRing();
        rings_[currentRing_]->TestBroadcast();
    }
}

void RandomFederationTestSession::ExecuteVoterStoreStage()
{
    SelectRing();
    rings_[currentRing_]->TestVoterStore();
}

void RandomFederationTestSession::AddUnreliableTransportBehavior(wstring const & command, bool userMode)
{
    wstring alias;
    if (userMode)
    {
        alias = wformatString("b{0}", unreliableTransportCommands_.size());
        AddInput(wformatString("addbehavior {0} {1}", alias, command));
    }
    else
    {
        alias = wformatString("k{0}", unreliableTransportCommands_.size());
        AddInput(wformatString("addleasebehavior {0} {1}", command, alias));
    }

    unreliableTransportCommands_.push_back(move(alias));
}

void RandomFederationTestSession::ClearUnreliableTransportBehavior()
{
    bool clearKernelTransportBehavior = false;
    for (wstring const & alias : unreliableTransportCommands_)
    {
        if (alias[0] == L'b')
        {
            AddInput(wformatString("removebehavior  {0}", alias));
        }
        else
        {
            clearKernelTransportBehavior = true;
        }
    }

    if (clearKernelTransportBehavior)
    {
        AddInput(FederationTestDispatcher::RemoveLeaseBehaviorCommand);
    }

    unreliableTransportCommands_.clear();
}
