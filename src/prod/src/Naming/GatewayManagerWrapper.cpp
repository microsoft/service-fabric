// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Hosting2/HostedServiceParameters.h"
#include "Hosting2/FabricActivatorClient.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Transport;

StringLiteral const TraceType_GatewayManager("GatewayManagerWrapper");

class GatewayManagerWrapper::ActivateAsyncOperation
    : public TimedAsyncOperation
{
public:
    ActivateAsyncOperation(
        GatewayManagerWrapper & owner,
        wstring const &ipcServerAddress,
        TimeSpan const &timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , ipcServerAddress_(ipcServerAddress)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisSPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);

        return thisSPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto operation = owner_.fabricGatewaySPtr_->BeginActivateGateway(
            ipcServerAddress_,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnFabricGatewayActivationComplete(operation, false);
            },
            thisSPtr);

        OnFabricGatewayActivationComplete(operation, true);
    }

private:

    void OnFabricGatewayActivationComplete(
        AsyncOperationSPtr const &operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fabricGatewaySPtr_->EndActivateGateway(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        if (!owner_.fabricApplicationGatewaySPtr_)
        {
            TryComplete(operation->Parent, error);
            return;
        }

        auto fabricAppGatewayActivateOperation = owner_.fabricApplicationGatewaySPtr_->BeginActivateGateway(
            ipcServerAddress_,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnFabricApplicationGatewayActivationComplete(operation, false);
            },
            operation->Parent);

        OnFabricApplicationGatewayActivationComplete(fabricAppGatewayActivateOperation, true);
    }

    void OnFabricApplicationGatewayActivationComplete(
        AsyncOperationSPtr const &operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fabricApplicationGatewaySPtr_->EndActivateGateway(operation);

        TryComplete(operation->Parent, error);
    }

    GatewayManagerWrapper &owner_;
    wstring ipcServerAddress_;
};

class GatewayManagerWrapper::DeactivateAsyncOperation
    : public TimedAsyncOperation
{
public:
    DeactivateAsyncOperation(
        GatewayManagerWrapper & owner,
        TimeSpan const &timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , deactivationError_(ErrorCodeValue::Success)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisSPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);

        return thisSPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        if (owner_.fabricApplicationGatewaySPtr_)
        {
            DeactivateFabricApplicationGateway(thisSPtr);
        }
        else
        {
            DeactivateFabricGateway(thisSPtr);
        }
    }

private:

    void DeactivateFabricGateway(AsyncOperationSPtr const &thisSPtr)
    {
        auto operation = owner_.fabricGatewaySPtr_->BeginDeactivateGateway(
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnFabricGatewayDeactivationComplete(operation, false);
            }, 
            thisSPtr);

        OnFabricGatewayDeactivationComplete(operation, true);
    }

    void DeactivateFabricApplicationGateway(AsyncOperationSPtr const &thisSPtr)
    {
        auto operation = owner_.fabricApplicationGatewaySPtr_->BeginDeactivateGateway(
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnFabricApplicationGatewayDeactivationComplete(operation, false);
            },
            thisSPtr);

            OnFabricApplicationGatewayDeactivationComplete(operation, true);
    }

    void OnFabricApplicationGatewayDeactivationComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        deactivationError_ = owner_.fabricApplicationGatewaySPtr_->EndDeactivateGateway(operation);
        if (!deactivationError_.IsSuccess())
        {
            WriteInfo(
                TraceType_GatewayManager,
                owner_.traceId_,
                "FabricApplicationGateway deactivation failed with {0}, this might be expected if we are closing fabrichost.",
                deactivationError_);
        }

        DeactivateFabricGateway(operation->Parent);
    }

    void OnFabricGatewayDeactivationComplete(
        AsyncOperationSPtr const &operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        deactivationError_ = ErrorCode::FirstError(deactivationError_, owner_.fabricGatewaySPtr_->EndDeactivateGateway(operation));

        TryComplete(operation->Parent, deactivationError_);
    }

    GatewayManagerWrapper &owner_;
    wstring ipcServerAddress_;
    ErrorCode deactivationError_;
};

IGatewayManagerSPtr GatewayManagerWrapper::CreateGatewayManager(
    FabricNodeConfigSPtr const & nodeConfig,
    IFabricActivatorClientSPtr const & activatorClient,
    ComponentRoot const & componentRoot)
{
    return shared_ptr<GatewayManagerWrapper>(new GatewayManagerWrapper(nodeConfig, activatorClient, componentRoot));
}

GatewayManagerWrapper::GatewayManagerWrapper(
    FabricNodeConfigSPtr const & nodeConfig,
    IFabricActivatorClientSPtr const & activatorClient,
    ComponentRoot const & componentRoot)
{
    traceId_ = componentRoot.TraceId;

    fabricGatewaySPtr_ = make_shared<FabricGatewayManager>(
        nodeConfig, 
        activatorClient, 
        componentRoot);

#if !defined(PLATFORM_UNIX)

    if (IsApplicationGatewayEnabled(nodeConfig, componentRoot))
    {
        fabricApplicationGatewaySPtr_ = make_shared<FabricApplicationGatewayManager>(
            nodeConfig,
            activatorClient,
            componentRoot);
    }

#endif
}

bool GatewayManagerWrapper::RegisterGatewaySettingsUpdateHandler()
{
    if (!fabricGatewaySPtr_->RegisterGatewaySettingsUpdateHandler())
    {
        return false;
    }

    if (fabricApplicationGatewaySPtr_)
    {
        return fabricApplicationGatewaySPtr_->RegisterGatewaySettingsUpdateHandler();
    }

    return true;
}

AsyncOperationSPtr GatewayManagerWrapper::BeginActivateGateway(
    wstring const &ipcServerAddress,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        ipcServerAddress,
        timeout,
        callback,
        parent);
}

ErrorCode GatewayManagerWrapper::EndActivateGateway(AsyncOperationSPtr const &operation)
{
    return ActivateAsyncOperation::End(operation);
}

AsyncOperationSPtr GatewayManagerWrapper::BeginDeactivateGateway(
    TimeSpan const&timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode GatewayManagerWrapper::EndDeactivateGateway(
    AsyncOperationSPtr const &operation)
{
    return DeactivateAsyncOperation::End(operation);
}

void GatewayManagerWrapper::AbortGateway()
{
    fabricGatewaySPtr_->AbortGateway();
    if (fabricApplicationGatewaySPtr_)
    {
        fabricApplicationGatewaySPtr_->AbortGateway();
    }
}

bool GatewayManagerWrapper::IsApplicationGatewayEnabled(
    FabricNodeConfigSPtr const &nodeConfig,
    ComponentRoot const & componentRoot)
{
#ifndef PLATFORM_UNIX
    if (HttpApplicationGatewayConfig::GetConfig().IsEnabled)
    {
        //
        // Application gateway can be enabled on the cluster, but it need not run on all node types.
        // So need not start the application gateway on a node if the port is not specified.
        //
        wstring portString;
        wstring hostString;
        auto error = TcpTransportUtility::TryParseHostPortString(
            nodeConfig->HttpApplicationGatewayListenAddress,
            hostString,
            portString);

        if (!error.IsSuccess())
        {
            WriteError(
                TraceType_GatewayManager,
                componentRoot.TraceId,
                "Failed to parse port from HttpApplicationGatewayListenAddress : {0}",
                nodeConfig->HttpGatewayListenAddress);

            return false;
        }

        if (portString == L"0")
        {
            WriteInfo(
                TraceType_GatewayManager,
                componentRoot.TraceId,
                "Application gateway not enabled on Node: {0}, because the http application gateway endpoint is specified as {1}",
                nodeConfig->NodeId,
                portString);

            return false;
        }

        return true;
    }
#endif
    return false;
}
