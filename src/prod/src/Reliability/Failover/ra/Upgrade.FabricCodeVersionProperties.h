// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            class FabricCodeVersionProperties
            {
            public:
                __declspec(property(get = get_IsDeactivationInfoSupported, put=set_IsDeactivationInfoSupported)) bool IsDeactivationInfoSupported;
                bool get_IsDeactivationInfoSupported() const { return isDeactivationInfoSupported_; } 
                void set_IsDeactivationInfoSupported(bool const & value) { isDeactivationInfoSupported_ = value; }

            private:
                bool isDeactivationInfoSupported_;
            };
        }
    }
}
