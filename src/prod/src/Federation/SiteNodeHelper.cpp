// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "SiteNodeHelper.h"

using namespace Federation;
using namespace Transport;
using namespace Common;
using namespace std;
using namespace FederationUnitTests;

wstring SiteNodeHelper::GetLoopbackAddress()
{
    return L"localhost";
}

wstring SiteNodeHelper::GetFederationPort()
{
    USHORT basePort = 0;
    TestPortHelper::GetPorts(1, basePort);

    wstring portString;
    StringWriter(portString).Write(basePort);
    return portString;
}

wstring SiteNodeHelper::GetLeaseAgentPort()
{
    USHORT basePort = 0;
    TestPortHelper::GetPorts(1, basePort);

    wstring portString;
    StringWriter(portString).Write(basePort);
    return portString;
}

wstring SiteNodeHelper::BuildAddress(wstring hostname, wstring port)
{
    wstring addr = hostname;
    addr.append(L":");
    addr.append(port);

    return addr;
}

SiteNodeSPtr SiteNodeHelper::CreateSiteNode(NodeId nodeId, wstring host, wstring port)
{
    wstring addr = BuildAddress(host, port);
    wstring laAddr = BuildAddress(host, GetLeaseAgentPort());
    NodeConfig config = NodeConfig(nodeId, addr, laAddr, GetWorkingDir());
    return SiteNode::Create(config, FabricCodeVersion(), nullptr);
}

SiteNodeSPtr SiteNodeHelper::CreateSiteNode(NodeId nodeId)
{
    wstring  port = GetFederationPort();
    wstring host = GetLoopbackAddress();
    return SiteNodeHelper::CreateSiteNode(nodeId, host, port);
}

wstring SiteNodeHelper::GetWorkingDir()
{
    wstring workingDir = L".\\Federation.Test\\Data";
    if(!Directory::Exists(workingDir))
    {
        Directory::Create(workingDir);
    }

    return workingDir;
}

wstring SiteNodeHelper::GetTicketFilePath(Federation::NodeId nodeId)
{
    return Path::Combine(GetWorkingDir(), nodeId.ToString() + L".tkt");
}

void SiteNodeHelper::DeleteTicketFile(Federation::NodeId nodeId)
{
    File::Delete(GetTicketFilePath(nodeId), false);
}
