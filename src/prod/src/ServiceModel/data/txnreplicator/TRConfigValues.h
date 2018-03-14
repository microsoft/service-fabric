// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class TRConfigValues :
        public Common::ComponentConfig,
        public TRConfigBase
    {
        DECLARE_COMPONENT_CONFIG(TRConfigValues, "Invalid");

        TR_CONFIG_PROPERTIES(L"Invalid");

        DEFINE_GET_TR_CONFIG_METHOD();
    };
}
