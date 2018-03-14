// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <memory>

namespace Federation
{
    class RequestReceiverContext;

    class ArbitrationOperation;
    typedef std::shared_ptr<ArbitrationOperation> ArbitrationOperationSPtr;
    class RequestReceiverContext;
    class OneWayReceiverContext;
	
    class FederationSubsystem;
    typedef std::shared_ptr<FederationSubsystem> FederationSubsystemSPtr;
    typedef std::unique_ptr<FederationSubsystem> FederationSubsystemUPtr;
    typedef std::unique_ptr<RequestReceiverContext> RequestReceiverContextUPtr;
    typedef std::unique_ptr<OneWayReceiverContext> OneWayReceiverContextUPtr;
}
