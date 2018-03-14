// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/SeedNodeProxy.h"

namespace Federation
{
    class WindowsAzureProxy : public SeedNodeProxy
    {
        DENY_COPY(WindowsAzureProxy);

    public:
        WindowsAzureProxy(NodeId voteId, std::wstring const & connectionString, std::wstring const & ringName, NodeId proxyId);

        // This function uses the seed info file to find the address corresponding to the
        // connection string and returns a NodeConfig corresponding to that address.
        static NodeConfig ResolveAzureEndPoint(NodeId voteId, std::wstring const & connectionString, std::wstring const & ringName, NodeId proxyId);

	protected:
        virtual NodeConfig GetConfig();

    private:
        std::wstring connectionString_;

        static Common::GlobalWString SeedMappingSection;
        static Common::GlobalWString SeedInfoFileName;
    };
}
