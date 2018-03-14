// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include "FederationTestCommonConfig.h"

using namespace std;
using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace TestCommon;
using namespace FederationTestCommon;

const StringLiteral TraceSource("AddressHelper"); 

//Change the range value in the comments in FabricTest.exe.cfg and FabricTest.Lab.exe.cfg if the values below change
USHORT AddressHelper::SeedNodeRangeStart = 12345;
USHORT AddressHelper::SeedNodeRangeEnd = 12375;

USHORT AddressHelper::StartPort = 13000;
USHORT AddressHelper::EndPort = 14000;

USHORT AddressHelper::ServerListenPort = 14011;

int AddressHelper::MaxRetryCount = 10;

double AddressHelper::RetryWaitTimeInSeconds = 5;

AddressHelper::AddressHelper()
{
    for(USHORT i = StartPort; i < EndPort; i++)
    {
        availablePorts_.insert(i);
    }

    RefreshAvailablePorts();
}

queue<USHORT> AddressHelper::GetAvailableSeedNodePorts()
{
    // Ports allocated to this process may not be available
    // 1. occupied by some "uncooperative" process
    // 2. occupied by a socket in wait states
    auto portsInUse = TestPortHelper::GetPortsInUse();

    queue<USHORT> availablePorts;
    for (USHORT port = SeedNodeRangeStart; port < SeedNodeRangeEnd; ++port)
    {
        if (portsInUse.find(port) == portsInUse.cend())
        {
            availablePorts.push(port);
        }
    }

    return availablePorts;
}

std::wstring AddressHelper::GetServerListenAddress()
{
    return AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ServerListenPort));
}

void AddressHelper::RefreshAvailablePorts()
{
    auto portsInUse = TestPortHelper::GetPortsInUse();
    for (auto port : portsInUse)
    {
        availablePorts_.erase(port.first);
    }
}

void AddressHelper::GetAvailablePorts(wstring const& nodeId, vector<USHORT> & ports, int numberOfPorts)
{
    AcquireExclusiveLock grab(portsLock_);
    if(usedPorts_.find(nodeId) != usedPorts_.end())
    {
        int expectedPorts = (numberOfPorts + 1);
        TestSession::FailTestIfNot(expectedPorts == static_cast<int>(usedPorts_[nodeId].size()), "Number of ports should always be {0} for an existing id {1}", expectedPorts, nodeId);
        ports = usedPorts_[nodeId];
    }
    else
    {
        RefreshAvailablePorts();

        TestSession::WriteInfo(TraceSource, "Requested {0} ports out of available {1} ports.", numberOfPorts, availablePorts_.size());
        TestSession::FailTestIf(static_cast<int>(availablePorts_.size()) < numberOfPorts, "Test ran out of available ports to use");

        for(int i = 0; i < numberOfPorts; i++)
        {
            auto iter = availablePorts_.begin();
            ports.push_back(*iter);
            availablePorts_.erase(iter);
        }

        ports.push_back(TestPortHelper::GetNextLeasePort());
        usedPorts_[nodeId] = ports;
    }

    TestSession::WriteInfo(TraceSource, "Ports assigned {0}.", ports);
}

wstring AddressHelper::GetLoopbackAddress()
{
    if (FederationTestCommonConfig::GetConfig().UseLoopbackAddress)
    {
        return L"127.0.0.1";
    }

    Endpoint ep; 
    auto error = TcpTransportUtility::GetFirstLocalAddress(ep, ResolveOptions::IPv4);
    Invariant(error.IsSuccess());
    return ep.GetIpString2(); 
}

wstring AddressHelper::BuildAddress(wstring const & port)
{
	return BuildAddress(GetLoopbackAddress(), port);
}

wstring AddressHelper::BuildAddress(wstring const & hostname, wstring const & port)
{
    wstring addr = hostname;
    addr.append(L":");
    addr.append(port);

    return addr;
}

