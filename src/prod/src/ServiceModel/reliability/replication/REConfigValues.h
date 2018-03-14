// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Reliability
{
    namespace ReplicationComponent
    { 
        class REConfigValues :
            public Common::ComponentConfig,
            public Reliability::ReplicationComponent::REConfigBase
        {
            DECLARE_COMPONENT_CONFIG(REConfigValues, "Invalid");

            RE_CONFIG_PROPERTIES(L"Invalid", 0, 0, 0, 0, 0);

            DEFINE_GETCONFIG_METHOD()
        };
    }
}
