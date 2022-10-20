// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/Failover.h"
#include "Reliability/Failover/FailoverPointers.h"

#include "Root.h"

namespace ModelChecker 
{
    class DummyHealthReportingTransport : public Client::HealthReportingTransport
    {
    public:
        DummyHealthReportingTransport(Common::ComponentRoot const& root)
            : HealthReportingTransport(root)
        {
        }

        virtual Common::AsyncOperationSPtr BeginReport(
            Transport::MessageUPtr && message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
        {
            message;timeout;
            return std::make_shared<Common::CompletedAsyncOperation>(Common::ErrorCodeValue::OperationCanceled, callback, parent);
        }

        virtual Common::ErrorCode EndReport(
            Common::AsyncOperationSPtr const & operation,
            __out Transport::MessageUPtr & reply)
        {
            reply;
            return Common::CompletedAsyncOperation::End(operation);
        }
    };
}
