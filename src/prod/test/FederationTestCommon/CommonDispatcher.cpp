// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Federation;
using namespace Common;
using namespace Transport;
using namespace TestCommon;
using namespace FederationTestCommon;

StringLiteral TraceOutput("Output");

GlobalWString DeclareVotesCommand = make_global<wstring>(L"votes");
GlobalWString ListBehaviorsCommand = make_global<wstring>(L"listbehaviors");
GlobalWString AddUnreliableTransportBehaviorCommand = make_global<wstring>(L"addbehavior");
GlobalWString RemoveUnreliableTransportBehaviorCommand = make_global<wstring>(L"removebehavior");

CommonDispatcher::CommonDispatcher()
{
}

bool CommonDispatcher::ExecuteCommand(wstring command)
{
    StringCollection paramCollection;
    StringUtility::Split<wstring>(command, paramCollection, L" ");
    if (paramCollection.size() == 0)
    {
        return false;
    }

    command = *paramCollection.begin();
    paramCollection.erase(paramCollection.begin());
    paramCollection = CompactParameters(paramCollection);

    if (command == DeclareVotesCommand)
    {
        return DeclareVotes(paramCollection);
    }
    else if (command == ListBehaviorsCommand)
    {
		return ListBehaviors();
    }
	else if (command == AddUnreliableTransportBehaviorCommand)
	{
		return AddUnreliableTransportBehavior(paramCollection);
	}
	else if (command == RemoveUnreliableTransportBehaviorCommand)
	{
		return RemoveUnreliableTransportBehavior(paramCollection);
	}

	return false;
}

bool CommonDispatcher::DeclareVotes(StringCollection const & params)
{
    VoteConfig voteOverrides;
    map<wstring, wstring> connectionStringsToVoteIdsMap;
    //TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    auto seedNodePorts = AddressHelper::GetAvailableSeedNodePorts();
    TestSession::FailTestIf(seedNodePorts.size() < params.size(), "seed node port address range should be larger than {0}", params.size());

    for (wstring const & nodeIdString : params)  
    {
        StringCollection configuration;
        StringUtility::Split<wstring>(nodeIdString, configuration, L":");

        wstring ringName;
        size_t index = configuration[0].find_first_of(L'@');
        if (index != wstring::npos)
        {
            ringName = configuration[0].substr(index + 1);
            configuration[0] = configuration[0].substr(0, index);
        }

        NodeId nodeId;
        wstring voteType, voteConnectionString;
        if (configuration.size() == 1)
        {
            nodeId = ParseNodeId(configuration[0]);
            voteType = *Federation::Constants::SeedNodeVoteType;
            StringWriter writer(voteConnectionString);
            writer.Write(AddressHelper::GetLoopbackAddress());
            writer.Write(":");
            writer.Write(seedNodePorts.front());
            seedNodePorts.pop();
        }
        else if (configuration.size() == 2)
        {
            voteType = configuration[1];
            if (voteType == Federation::Constants::SeedNodeVoteType)
            {
                nodeId = ParseNodeId(configuration[0]);
                StringWriter writer(voteConnectionString);
                writer.Write(AddressHelper::GetLoopbackAddress());
                writer.Write(":");
                writer.Write(seedNodePorts.front());
                seedNodePorts.pop();
            }
            else
            {
                ErrorCode errorCode = NodeIdGenerator::GenerateFromString(configuration[0], nodeId);
                TestSession::FailTestIfNot(errorCode.IsSuccess(), "NodeIdGenerator failed");
                voteConnectionString = L"Driver={SQL Server};Server=(local);Database=master;Trusted_Connection=yes";
            }
        }
        else if (configuration.size() == 3)
        {
            voteType = configuration[1];
            if (voteType == Federation::Constants::SeedNodeVoteType)
            {
                nodeId = ParseNodeId(configuration[0]);
                voteConnectionString = AddressHelper::GetLoopbackAddress() + L":" + configuration[2];
            }
            else
            {
                ErrorCode errorCode = NodeIdGenerator::GenerateFromString(configuration[0], nodeId);
                TestSession::FailTestIfNot(errorCode.IsSuccess(), "NodeIdGenerator failed");
                voteConnectionString = configuration[2];
                TestSession::FailTestIf(
                    !connectionStringsToVoteIdsMap.empty() && connectionStringsToVoteIdsMap.find(voteConnectionString) != connectionStringsToVoteIdsMap.end(),
                    "Connection string {0} specified for key {1} already",
                    voteConnectionString, connectionStringsToVoteIdsMap[voteConnectionString]);
                connectionStringsToVoteIdsMap[voteConnectionString] = configuration[0];
            }
        }
        else
        {
            TestSession::FailTest("Invalid vote configuration");
        }
        TestSession::FailTestIf(voteOverrides.find(nodeId, ringName) != voteOverrides.end(), "Vote {0} for key {1} already present", nodeId, configuration[0]);

        voteOverrides.push_back(VoteEntryConfig(nodeId, voteType, voteConnectionString, ringName));
    }

    FederationConfig::GetConfig().Votes = voteOverrides;

    return true;
}

bool CommonDispatcher::AddUnreliableTransportBehavior(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        PrintHelp(*AddUnreliableTransportBehaviorCommand);
        return true;
    }

    wstring name = params[0];
    wstring data;
    for (size_t i = 1; i < params.size(); i++)
    {
        if (i > 1)
        {
            data += L" ";
        }
        data += params[i];
    }

    if (!UnreliableTransportConfig::GetConfig().AddSpecification(name, data))
    {
        TestSession::WriteError(TraceOutput, "Invalid unreliable transport specification.");
    }

    return true;
}

bool CommonDispatcher::RemoveUnreliableTransportBehavior(Common::StringCollection const & params)
{
    if (params.size() != 1)
    {
        PrintHelp(*RemoveUnreliableTransportBehaviorCommand);
        return true;
    }

    UnreliableTransportConfig::GetConfig().RemoveSpecification(params[0]);

	return true;
}

bool CommonDispatcher::ListBehaviors()
{
    TestSession::WriteInfo(TraceOutput, "{0}", UnreliableTransportConfig::GetConfig());
    return true;
}

NodeId CommonDispatcher::ParseNodeId(wstring const & nodeIdString)
{
    return NodeId(ParseLargeInteger(nodeIdString));
}

LargeInteger CommonDispatcher::ParseLargeInteger(wstring const & value)
{
    LargeInteger result;
    if (!LargeInteger::TryParse(value, result))
    {
        TestSession::FailTest("Cannot parse a large integer {0}", value);
    }

    return result;
}
