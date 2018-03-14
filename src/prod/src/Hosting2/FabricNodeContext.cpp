// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

FabricNodeContext::FabricNodeContext()
    : nodeId_(),
    clientConnectionAddress_(),
    runtimeConnectionAddress_(),
    deploymentDirectory_(),
    nodeInstanceId_(0),
    nodeName_(),
    nodeType_(),
    ipAddressOrFQDN_(),
    logicalApplicationDirectories_(),
    logicalNodeDirectories_()
{
}

FabricNodeContext::FabricNodeContext(
    wstring const & nodeName,
    wstring const & nodeId,
    uint64 nodeInstanceId,
    wstring const & nodeType,
    wstring const & clientConnectionAddress, 
    wstring const & runtimeConnectionAddress,
    wstring const & deploymentDirectory,
    wstring const & ipAddressOrFQDN,
    wstring const & nodeWorkFolder,
    std::map<wstring, wstring> const& logicalApplicationDirectories,
    std::map<wstring, wstring> const& logicalNodeDirectories)
    : nodeId_(nodeId),
    clientConnectionAddress_(clientConnectionAddress),
    runtimeConnectionAddress_(runtimeConnectionAddress),
    deploymentDirectory_(deploymentDirectory),
    nodeInstanceId_(nodeInstanceId),
    nodeName_(nodeName),
    nodeType_(nodeType),
    ipAddressOrFQDN_(ipAddressOrFQDN),
    nodeWorkFolder_(nodeWorkFolder),
    logicalApplicationDirectories_(logicalApplicationDirectories),
    logicalNodeDirectories_(logicalNodeDirectories)
{
}


FabricNodeContext::FabricNodeContext(FabricNodeContext const & other)
    : nodeId_(other.nodeId_),
    clientConnectionAddress_(other.clientConnectionAddress_),
    runtimeConnectionAddress_(other.runtimeConnectionAddress_),
    deploymentDirectory_(other.deploymentDirectory_),
    nodeInstanceId_(other.nodeInstanceId_),
    nodeName_(other.nodeName_),
    nodeType_(other.nodeType_),
    ipAddressOrFQDN_(other.ipAddressOrFQDN_),
    nodeWorkFolder_(other.nodeWorkFolder_),
    logicalApplicationDirectories_(other.logicalApplicationDirectories_),
    logicalNodeDirectories_(other.logicalNodeDirectories_)
{
}

FabricNodeContext::FabricNodeContext(FabricNodeContext && other)
    : nodeId_(move(other.nodeId_)),
    clientConnectionAddress_(move(other.clientConnectionAddress_)),
    runtimeConnectionAddress_(move(other.runtimeConnectionAddress_)),
    deploymentDirectory_(move(other.deploymentDirectory_)),
    nodeInstanceId_(other.nodeInstanceId_),
    nodeName_(move(other.nodeName_)),
    nodeType_(move(other.nodeType_)),
    ipAddressOrFQDN_(move(other.ipAddressOrFQDN_)),
    nodeWorkFolder_(move(other.nodeWorkFolder_)),
    logicalApplicationDirectories_(move(other.logicalApplicationDirectories_)),
    logicalNodeDirectories_(move(other.logicalNodeDirectories_))
{
}

FabricNodeContext const & FabricNodeContext::operator = (FabricNodeContext const & other)
{
    if (this != &other)
    {
        this->nodeId_ = other.nodeId_;
        this->clientConnectionAddress_ = other.clientConnectionAddress_;
        this->runtimeConnectionAddress_ = other.runtimeConnectionAddress_;
        this->deploymentDirectory_ = other.deploymentDirectory_;
        this->nodeInstanceId_ = other.nodeInstanceId_;
        this->nodeName_ = other.nodeName_;
        this->nodeType_ = other.nodeType_;
        this->ipAddressOrFQDN_ = other.ipAddressOrFQDN_;
        this->nodeWorkFolder_ = other.nodeWorkFolder_;
        this->logicalApplicationDirectories_ = other.logicalApplicationDirectories_;
        this->logicalNodeDirectories_ = other.logicalNodeDirectories_;
    }

    return *this;
}

FabricNodeContext const & FabricNodeContext::operator = (FabricNodeContext && other)
{
    if (this != &other)
    {
        this->nodeId_ = move(other.nodeId_);
        this->clientConnectionAddress_ = move(other.clientConnectionAddress_);
        this->runtimeConnectionAddress_ = move(other.runtimeConnectionAddress_);
        this->deploymentDirectory_ = move(other.deploymentDirectory_);
        this->nodeInstanceId_ = other.nodeInstanceId_;
        this->nodeName_ = move(other.nodeName_);
        this->nodeType_ = move(other.nodeType_);
        this->ipAddressOrFQDN_ = move(other.ipAddressOrFQDN_);
        this->nodeWorkFolder_ = move(other.nodeWorkFolder_);
        this->logicalApplicationDirectories_ = move(other.logicalApplicationDirectories_);
        this->logicalNodeDirectories_ = move(other.logicalNodeDirectories_);
    }

    return *this;
}

bool FabricNodeContext::operator == (FabricNodeContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeId_, other.nodeId_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->clientConnectionAddress_, other.clientConnectionAddress_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->runtimeConnectionAddress_, other.runtimeConnectionAddress_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->deploymentDirectory_, other.deploymentDirectory_);
    if (!equals) { return equals; }

    equals = (this->nodeInstanceId_ == other.nodeInstanceId_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeName_, other.nodeName_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeType_, other.nodeType_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ipAddressOrFQDN_, other.ipAddressOrFQDN_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->nodeWorkFolder_, other.nodeWorkFolder_);
    if (!equals) { return equals; }

    equals = (this->logicalApplicationDirectories_ == other.logicalApplicationDirectories_);
    if (!equals) { return equals; }

    equals = (this->logicalNodeDirectories_ == other.logicalNodeDirectories_);
    if (!equals) { return equals; }

    return equals;
}

bool FabricNodeContext::operator != (FabricNodeContext const & other) const
{
    return !(*this == other);
}

void FabricNodeContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FabricNodeContext { ");
    w.Write("NodeName = {0}", nodeName_);
    w.Write("NodeId = {0}, ", nodeId_);
    w.Write("NodeInstanceId = {0}", nodeInstanceId_);
    w.Write("ClientConnectionAddress = {0}, ", clientConnectionAddress_);
    w.Write("RuntimeConnectionAddress = {0}, ", runtimeConnectionAddress_);
    w.Write("DeploymentDirectory = {0}, ", deploymentDirectory_);
    w.Write("NodeType = {0}", nodeType_);
    w.Write("IPAddressOrFQDN = {0}", ipAddressOrFQDN_);
    w.Write("NodeWorkFolder = {0}", nodeWorkFolder_);
    if (!logicalApplicationDirectories_.empty())
    {
        w.WriteLine("LogicalApplicationDirectories = ");
        for (auto const& item : logicalApplicationDirectories_)
        {
            w.WriteLine("{0}, {1}", item.first, item.second);
        }
    }
    if (!logicalNodeDirectories_.empty())
    {
        w.WriteLine("LogicalNodeDirectories = ");
        for (auto const& item : logicalNodeDirectories_)
        {
            w.WriteLine("{0}, {1}", item.first, item.second);
        }
    }
    w.Write("}");
}

void FabricNodeContext::Test_SetLogicalApplicationDirectories(std::map<wstring, wstring> && logicalApplicationDirectories)
{
    if (!logicalApplicationDirectories.empty())
    {
        this->logicalApplicationDirectories_ = move(logicalApplicationDirectories);
    }
}
