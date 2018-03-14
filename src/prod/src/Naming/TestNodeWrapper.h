// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "RouterTestHelper.h"

namespace Naming
{
    namespace TestHelper
    {
        class TestNodeWrapper : public Reliability::FederationWrapper
        {
            DENY_COPY(TestNodeWrapper);

        public:
            TestNodeWrapper(__in Federation::FederationSubsystem & federation, __in RouterTestHelper & transport)
                : Reliability::FederationWrapper(federation, nullptr),
              transport_(transport)
            {
            }            

            Common::AsyncOperationSPtr BeginRequestToFM(
                Transport::MessageUPtr && message,
                Common::TimeSpan const retryInterval,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndRequestToFM(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply);

            Common::AsyncOperationSPtr BeginRequestToNode(
                Transport::MessageUPtr && message,
                Federation::NodeInstance const & target,
                bool useExactRouting,
                Common::TimeSpan const retryInterval,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) const;
            Common::ErrorCode EndRequestToNode(Common::AsyncOperationSPtr const & operation, __out Transport::MessageUPtr & reply) const;

        private:
            RouterTestHelper & transport_;
        };
    }
}
