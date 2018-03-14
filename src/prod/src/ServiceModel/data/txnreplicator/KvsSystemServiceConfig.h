// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // This class only exists in order to read the default values from SLConfigMacros.h
    // in KtlLoggerSharedLogSettings for system services running on the managed KVS stack
    // (i.e. FabricUS).
    //
    class KvsSystemServiceConfig 
        : public Common::ComponentConfig
        , public SLConfigBase
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(KvsSystemServiceConfig, "KvsSystemServiceConfig")

        SL_CONFIG_PROPERTIES(L"KvsSystemServiceConfig/SharedLog");
    };
}

