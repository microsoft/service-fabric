// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            This is a wrapper around IFederationWrapper and exposes only the logic needed by RA
         */
        class FederationTransport
        {
            DENY_COPY(FederationTransport);
        public:

            template<typename T>
            void SendMessageToFM()
        };
    }
}



