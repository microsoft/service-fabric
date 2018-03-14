// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class SLConfigValues :
        public Common::ComponentConfig,
        public SLConfigBase
    {
        DECLARE_COMPONENT_CONFIG(SLConfigValues, "Invalid");

        SL_CONFIG_PROPERTIES(L"Invalid");
    };
}
