// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class TransactionalReplicatorConfig :
        public Common::ComponentConfig,
        public TRConfigBase
    {
        DECLARE_COMPONENT_CONFIG(TransactionalReplicatorConfig, "TransactionalReplicator2");
        
        TR_CONFIG_PROPERTIES(L"TransactionalReplicator2");

        DEFINE_GET_TR_CONFIG_METHOD();
    };

    typedef std::shared_ptr<TransactionalReplicatorConfig> TRConfigSPtr;
}
