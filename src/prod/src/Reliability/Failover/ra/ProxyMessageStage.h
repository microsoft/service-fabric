// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents the message stage on an RAP replica
        namespace ProxyMessageStage
        {
            enum Enum
            {
                None = 0,
                RAProxyReportFaultTransient = 1,
                RAProxyReportFaultPermanent = 2,
                RAProxyEndpointUpdated = 3,
                RAProxyReadWriteStatusRevoked = 4,

                LastValidEnum = RAProxyReadWriteStatusRevoked
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ProxyMessageStage);
        }
    }
}
