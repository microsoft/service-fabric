// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestNodeWrapper.h"

using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace Reliability;

namespace Naming
{
    namespace TestHelper
    {
        AsyncOperationSPtr TestNodeWrapper::BeginRequestToFM(
            MessageUPtr && message,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            UNREFERENCED_PARAMETER(retryInterval);

            return transport_.BeginRouteRequest(std::move(message), NodeId::MinNodeId, 0, true, timeout, callback, parent);
        }

        ErrorCode TestNodeWrapper::EndRequestToFM(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
        {
            return transport_.EndRouteRequest(operation, reply);
        }

        AsyncOperationSPtr TestNodeWrapper::BeginRequestToNode(
            MessageUPtr && message,
            NodeInstance const & target,
            bool useExactRouting,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) const
        {
            UNREFERENCED_PARAMETER(retryInterval);

            return transport_.BeginRouteRequest(std::move(message), target.Id, target.InstanceId, useExactRouting, timeout, callback, parent); 
        }

        ErrorCode TestNodeWrapper::EndRequestToNode(AsyncOperationSPtr const & operation, __out MessageUPtr & reply) const
        {
            return transport_.EndRouteRequest(operation, reply);
        }
    }
}
