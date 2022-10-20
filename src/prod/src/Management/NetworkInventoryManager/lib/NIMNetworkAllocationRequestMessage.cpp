// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::NetworkInventoryManager;

const std::wstring NetworkAllocationRequestMessage::ActionName = L"NetworkAllocationRequestMessage";
const std::wstring NetworkCreateRequestMessage::ActionName = L"NetworkCreateRequestMessage";
const std::wstring NetworkRemoveRequestMessage::ActionName = L"NetworkRemoveRequestMessage";
const std::wstring NetworkEnumerateRequestMessage::ActionName = L"NetworkEnumerateRequestMessage";
const std::wstring PublishNetworkTablesMessageRequest::ActionName = L"PublishNetworkTablesMessage";


void NetworkAllocationRequestMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.WriteLine("NetworkAllocationRequestMessage: Network: [{0}], Node: {1}, InfraIP: [{2}]", 
        networkName_, nodeName_, infraIpAddress_);
}

void NetworkCreateRequestMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "NetworkCreateRequestMessage " << networkName_ ;
}

void NetworkRemoveRequestMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "NetworkRemoveRequestMessage " << networkId_ ;
}

void NetworkEnumerateRequestMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "NetworkEnumerateRequestMessage: " << networkNameFilter_ ;
}

void PublishNetworkTablesMessageRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.WriteLine("PublishNetworkTablesMessageRequest: NetworkId: [{0}], NodeId: [{1}]", networkId_, nodeId_);
}
