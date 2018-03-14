// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestClientsTracker
    {
        DENY_COPY(FabricTestClientsTracker)

    public:

        FabricTestClientsTracker(__in FabricTestFederation &);

        virtual ~FabricTestClientsTracker();

        std::shared_ptr<Common::FabricNodeConfig> GetFabricNodeConfig(std::wstring const &);
        Api::IClientFactoryPtr CreateClientFactoryOnNode(std::wstring const & nodeIdString);
        Api::IClientFactoryPtr CreateCachedClientFactoryOnNode(CommandLineParser const &);

    private:
        FabricTestFederation & testFederation_;
        std::unordered_map<std::wstring, Api::IClientFactoryPtr> clientFactories_;
    };
}
