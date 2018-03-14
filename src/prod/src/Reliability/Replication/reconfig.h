// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class REConfig : 
            Common::ComponentConfig
        {
            DECLARE_COMPONENT_CONFIG(REConfig, "RE");

            RE_CONFIG_PROPERTIES(L"Replication", 15, 1024, 0, 2048, 0); 
       };
    }
}

