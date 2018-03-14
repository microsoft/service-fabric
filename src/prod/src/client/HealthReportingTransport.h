// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class HealthReportingTransport : public Common::RootedObject
    {
    protected:
        HealthReportingTransport(Common::ComponentRoot const & root)
            : Common::RootedObject(root)
        {
        }

    public:
        virtual Common::AsyncOperationSPtr BeginReport(
            Transport::MessageUPtr && message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0; 
        virtual Common::ErrorCode EndReport(
            Common::AsyncOperationSPtr const & operation,
            __out Transport::MessageUPtr & reply) = 0;
    };
}
