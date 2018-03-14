// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/WindowsAzureProxy.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace LeaseWrapper;

GlobalWString WindowsAzureProxy::SeedMappingSection = make_global<wstring>(L"SeedMapping");
GlobalWString WindowsAzureProxy::SeedInfoFileName = make_global<wstring>(L"SeedInfo.ini");

WindowsAzureProxy::WindowsAzureProxy(NodeId voteId, wstring const & connectionString, wstring const & ringName, NodeId proxyId)
    :   connectionString_(connectionString),
        SeedNodeProxy(ResolveAzureEndPoint(voteId, connectionString, ringName, proxyId), proxyId, false)
{
}

NodeConfig WindowsAzureProxy::GetConfig()
{
    return ResolveAzureEndPoint(VoteId, connectionString_, GetRingName(), ProxyId);
}

NodeConfig WindowsAzureProxy::ResolveAzureEndPoint(NodeId voteId, wstring const & connectionString, wstring const & ringName, NodeId proxyId)
{
    wstring address;
    try
    {
        bool isEncrypted;
	    wstring fabricDataRoot;
	    ErrorCode errorCode = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        ASSERT_IF(!errorCode.IsSuccess(), "Unable to get FabricDataRoot");
        FileConfigStore file(Path::Combine(fabricDataRoot,SeedInfoFileName));
        address = file.ReadString(SeedMappingSection, connectionString, isEncrypted);     
        ASSERT_IF(isEncrypted, "FileConfigStore does not support encrypted values");
    }
    catch (std::system_error e)
    {
        Assert::CodingError("Unable to access seed info file because {0}", e);
    }

    ASSERT_IF(address == L"", "The connection string has no mapping in the file");
    WriteInfo("Resolve", "AzureProxy {0} resolved to {1} on {2}",
        voteId, address, proxyId);

    return NodeConfig(voteId, address, L"", L"", ringName);
}
