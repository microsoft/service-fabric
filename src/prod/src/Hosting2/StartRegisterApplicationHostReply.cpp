// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StartRegisterApplicationHostReply::StartRegisterApplicationHostReply()
    : nodeId_(),
    nodeInstanceId_(0),
    nodeName_(),
    nodeHostProcessId_(0),
    nodeType_(),
    ipAddressOrFQDN_(),
    nodeLeaseHandle_(0),
    nodeLeaseDuration_(),
    errorCode_(),
    clientConnectionAddress_(),
    deploymentDirectory_(),
    nodeWorkFolder_(),
    logicalApplicationDirectories_(),
    logicalNodeDirectories_(),
    applicationSharedLogSettings_(make_shared<KtlLogger::SharedLogSettings>()),
    systemServicesSharedLogSettings_(make_shared<KtlLogger::SharedLogSettings>())
{
}

StartRegisterApplicationHostReply::StartRegisterApplicationHostReply(
    wstring const & nodeId,
    uint64 nodeInstanceId,
    wstring const & nodeName,
    DWORD nodeHostProcessId,
    wstring const & nodeType,
    wstring const & ipAddressOrFQDN,
    std::wstring const & clientConnectionAddress,
    std::wstring const & deploymentDirectory,
    HANDLE nodeLeaseHandle,
    TimeSpan initialLeaseTTL,
    TimeSpan const & nodeLeaseDuration,
    ErrorCode const & errorCode,
    std::wstring const &nodeWorkFolder,
    std::map<std::wstring, std::wstring> const& logicalApplicationDirectories,
    std::map<std::wstring, std::wstring> const& logicalNodeDirectories,
    KtlLogger::SharedLogSettingsSPtr applicationSharedLogSettings,
    KtlLogger::SharedLogSettingsSPtr systemServicesSharedLogSettings)
    : nodeId_(nodeId),
    nodeInstanceId_(nodeInstanceId),
    nodeName_(nodeName),
    nodeHostProcessId_(nodeHostProcessId),
    nodeType_(nodeType),
    ipAddressOrFQDN_(ipAddressOrFQDN),
    initialLeaseTTL_(initialLeaseTTL),
    nodeLeaseHandle_(reinterpret_cast<uint64>(nodeLeaseHandle)),
    nodeLeaseDuration_(nodeLeaseDuration),
    errorCode_(errorCode),
    clientConnectionAddress_(clientConnectionAddress),
    deploymentDirectory_(deploymentDirectory),
    nodeWorkFolder_(nodeWorkFolder),
    logicalApplicationDirectories_(logicalApplicationDirectories),
    logicalNodeDirectories_(logicalNodeDirectories),
    applicationSharedLogSettings_(applicationSharedLogSettings),
    systemServicesSharedLogSettings_(systemServicesSharedLogSettings)
{
}

void StartRegisterApplicationHostReply::WriteTo(TextWriter & w, FormatOptions const & f) const
{
    w.Write("StartRegisterApplicationHostReply { ");
    w.Write("NodeId = {0}, ", NodeId);
    w.Write("NodeInstanceId = {0}, ", nodeInstanceId_);
    w.Write("NodeHostProcessId = {0}, ", NodeHostProcessId);
    w.Write("NodeType = {0} ", nodeType_);
    w.Write("IPAddressOrFQDN = {0} ", ipAddressOrFQDN_);
    w.Write("ClientConnectionAddress = {0} ", clientConnectionAddress_);
    w.Write("DeploymentDirectory = {0} ", deploymentDirectory_);
    w.Write("NodeLeaseHandle = {0}, ", (uint64)NodeLeaseHandle);
    w.Write("NodeLeaseDuration = {0}, ", NodeLeaseDuration);
    w.Write("ErrorCode = {0}, ", Error);
    w.Write("NodeWorkFolder = {0}, ", NodeWorkFolder);
    w.Write("InitialLeaseTTL= {0}", InitialLeaseTTL());

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

    if (applicationSharedLogSettings_ != nullptr)
    {
        w.Write("Application");
        applicationSharedLogSettings_->WriteTo(w, f);
    }
    else
    {
        w.Write("ApplicationSharedLogSettings = nullptr");
    }
    if (systemServicesSharedLogSettings_ != nullptr)
    {
        w.Write("SystemServices");
        systemServicesSharedLogSettings_->WriteTo(w, f);
    }
    else
    {
        w.Write("SystemServicesSharedLogSettings = nullptr");
    }

    w.Write("}");
}
