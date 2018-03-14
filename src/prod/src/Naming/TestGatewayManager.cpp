// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "httpgateway/HttpGatewayConfig.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Naming;
using namespace HttpGateway;
using namespace FabricGateway;

StringLiteral const TraceType_GatewayManager("TestGatewayManager");

TestGatewayManager::TestGatewayManager(
    FabricNodeConfigSPtr const & nodeConfig,
    ComponentRoot const & componentRoot)
    : RootedObject(componentRoot)
    , nodeConfig_(nodeConfig)
    , ipcListenAddress_()
{
}

IGatewayManagerSPtr TestGatewayManager::CreateGatewayManager(
    FabricNodeConfigSPtr const &nodeConfig,
    ComponentRoot const &root)
{
    return shared_ptr<TestGatewayManager>(new TestGatewayManager(nodeConfig, root));
}

bool TestGatewayManager::RegisterGatewaySettingsUpdateHandler()
{
    return true;
}

AsyncOperationSPtr TestGatewayManager::BeginActivateGateway(
    wstring const &ipcServerAddress,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    ipcListenAddress_ = ipcServerAddress;

    Gateway::Create(nodeConfig_, ipcServerAddress, fabricGatewaySPtr_);
    return fabricGatewaySPtr_->BeginOpen(
        timeout,
        callback,
        parent);
}

ErrorCode TestGatewayManager::EndActivateGateway(AsyncOperationSPtr const &operation)
{
    return fabricGatewaySPtr_->EndOpen(operation);
}

AsyncOperationSPtr TestGatewayManager::BeginDeactivateGateway(
    TimeSpan const&timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return fabricGatewaySPtr_->BeginClose(timeout, callback, parent);
}

ErrorCode TestGatewayManager::EndDeactivateGateway(
    AsyncOperationSPtr const &operation)
{
    return fabricGatewaySPtr_->EndClose(operation);
}

void TestGatewayManager::AbortGateway()
{
    fabricGatewaySPtr_->Abort();
}
