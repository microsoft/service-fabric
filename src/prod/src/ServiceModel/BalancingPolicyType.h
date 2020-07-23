// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace BalancingPolicyType
    {
        enum Enum
        {
            Global = 0,
            Isolated = 1,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        BalancingPolicyType::Enum GetBalancingPolicyType(std::wstring const & val);
        FABRIC_SERVICE_LOAD_METRIC_BALANCING_POLICY ToPublicApi(__in BalancingPolicyType::Enum const & balancingPolicy);
    };
}
