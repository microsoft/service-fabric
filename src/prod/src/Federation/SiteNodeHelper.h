// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationUnitTests
{
    class SiteNodeHelper
    {
    public:
        static Federation::SiteNodeSPtr CreateSiteNode(Federation::NodeId nodeId, std::wstring host, std::wstring port);
        static Federation::SiteNodeSPtr CreateSiteNode(Federation::NodeId nodeId);
        static std::wstring GetLoopbackAddress();
        static std::wstring GetFederationPort();
        static std::wstring GetLeaseAgentPort();
        static std::wstring BuildAddress(std::wstring hostname, std::wstring port);
        static std::wstring GetTicketFilePath(Federation::NodeId nodeId);
        static void DeleteTicketFile(Federation::NodeId nodeId);
        static std::wstring GetWorkingDir();
    };
};

