// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ComContainerActivatorServiceAgent");

ComContainerActivatorServiceAgent::ComContainerActivatorServiceAgent(
    IContainerActivatorServiceAgentPtr const & impl)
    : IFabricContainerActivatorServiceAgent2()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComContainerActivatorServiceAgent::~ComContainerActivatorServiceAgent()
{
    // Break lifetime cycle between ContainerActivatorServiceAgent and IpcServer
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComContainerActivatorServiceAgent::ProcessContainerEvents(
    FABRIC_CONTAINER_EVENT_NOTIFICATION *notifiction)
{
    ContainerEventNotification notification;
    auto error = notification.FromPublicApi(*notifiction);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "Failed to parse ContainerEventNotification from FABRIC_CONTAINER_EVENT_NOTIFICATION. Error={0}",
            error);

        return ComUtility::OnPublicApiReturn(error);
    }

    impl_->ProcessContainerEvents(move(notification));

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComContainerActivatorServiceAgent::RegisterContainerActivatorService(
    IFabricContainerActivatorService * activatorService)
{
    UNREFERENCED_PARAMETER(activatorService);

    return ComUtility::OnPublicApiReturn(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE ComContainerActivatorServiceAgent::RegisterContainerActivatorService(
    IFabricContainerActivatorService2 * activatorService)
{
    auto rootedPtr = WrapperFactory::create_rooted_com_proxy(activatorService);

    auto error = impl_->RegisterContainerActivatorService(rootedPtr);

    return ComUtility::OnPublicApiReturn(error);
}
