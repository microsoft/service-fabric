// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// FabricNodeContextResult::FabricNodeContextResult Implementation
//

FabricNodeContextResult::FabricNodeContextResult(
    wstring const & nodeId,
    uint64 nodeInstanceId,
    wstring const & nodeName,
    wstring const & nodeType,
    wstring const & ipAddressOrFQDN,
    map<wstring, wstring> const& logicalNodeDirectories)
    : ComponentRoot(),
    nodeId_(nodeId),
    nodeInstanceId_(nodeInstanceId),
    nodeName_(nodeName),
    nodeType_(nodeType),
    ipAddressOrFQDN_(ipAddressOrFQDN),
    logicalNodeDirectories_(logicalNodeDirectories)
{
    nodeContext_.Reserved = NULL;

    Common::LargeInteger nodeID;
    Common::LargeInteger::TryParse(nodeId_, nodeID);

    nodeContext_.NodeId.High = nodeID.High;
    nodeContext_.NodeId.Low = nodeID.Low;
    nodeContext_.NodeInstanceId = nodeInstanceId_;
    nodeContext_.NodeName = nodeName_.c_str();
    nodeContext_.NodeType = nodeType_.c_str();
    nodeContext_.IPAddressOrFQDN = ipAddressOrFQDN_.c_str();
}

FabricNodeContextResult::~FabricNodeContextResult()
{
}

Common::ErrorCode FabricNodeContextResult::GetDirectory(
    std::wstring const& logicalDirectoryName,
    __out std::wstring & directoryPath)
{

    auto iter = logicalNodeDirectories_.find(logicalDirectoryName);
    if (iter != logicalNodeDirectories_.end())
    {
        directoryPath = iter->second;
        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::DirectoryNotFound;
}

const FABRIC_NODE_CONTEXT * FabricNodeContextResult::get_NodeContext(void)
{
    return &nodeContext_;
}
