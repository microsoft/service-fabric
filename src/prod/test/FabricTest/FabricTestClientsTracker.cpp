// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace FabricTest;

FabricTestClientsTracker::FabricTestClientsTracker(__in FabricTestFederation & testFederation)
    : testFederation_(testFederation)
    , clientFactories_()
{
}

FabricTestClientsTracker::~FabricTestClientsTracker()
{
}

shared_ptr<Common::FabricNodeConfig> FabricTestClientsTracker::GetFabricNodeConfig(wstring const & nodeIdString)
{
    auto nodeId = FederationTestCommon::CommonDispatcher::ParseNodeId(nodeIdString);
    auto node = testFederation_.GetNode(nodeId);
    if (!node)
    {
        TestSession::FailTest("Node {0} does not exist", nodeId);
    }

    shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
    bool success = node->TryGetFabricNodeWrapper(fabricNodeWrapper);
    TestSession::FailTestIfNot(success, "TryGetFabricNodeWrapper for node id {0} should be for an in proc node", nodeId);

    return fabricNodeWrapper->Config;
}

Api::IClientFactoryPtr FabricTestClientsTracker::CreateClientFactoryOnNode(wstring const & nodeIdString)
{
    auto nodeConfig = this->GetFabricNodeConfig(nodeIdString);

    IClientFactoryPtr factoryPtr;
    auto error = Client::ClientFactory::CreateLocalClientFactory(
        nodeConfig,
        factoryPtr);
    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create local fabric client factory error = {0}", error);

    return factoryPtr;
}

Api::IClientFactoryPtr FabricTestClientsTracker::CreateCachedClientFactoryOnNode(CommandLineParser const & parser)
{
    wstring clientName;
    parser.TryGetString(L"client", clientName, L"");

    wstring nodeName;
    parser.TryGetString(L"node", nodeName, L"");

    IClientFactoryPtr clientFactory;
    if (clientName.empty())
    {
        if (nodeName.empty())
        {
            TestSession::FailTest("Must provide 'node' for unnamed client");
        }

        clientFactory = this->CreateClientFactoryOnNode(nodeName);
    }
    else
    {
        auto findIt = clientFactories_.find(clientName);
        if (findIt == clientFactories_.end())
        {
            if (nodeName.empty())
            {
                TestSession::FailTest("Must provide 'node' for new client: {0}", clientName);
            }

            clientFactory = this->CreateClientFactoryOnNode(nodeName);

            IClientSettingsPtr clientSettings;
            auto error = clientFactory->CreateSettingsClient(clientSettings);

            if (!error.IsSuccess())
            {
                TestSession::FailTest("CreateSettingsClient: {0}", error);
            }

            auto settings = clientSettings->GetSettings();
            settings.SetClientFriendlyName(clientName);
            error = clientSettings->SetSettings(move(settings));

            if (!error.IsSuccess())
            {
                TestSession::FailTest("SetSettings: {0}", error);
            }

            clientFactories_.insert(make_pair(clientName, clientFactory));
        }
        else
        {
            clientFactory = findIt->second;
        }
    }

    return clientFactory;
}
