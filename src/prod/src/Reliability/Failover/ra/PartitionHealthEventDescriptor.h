// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {        
        class PartitionHealthEventDescriptor : public Health::HealthReportDescriptorBase
        {
            DENY_COPY(PartitionHealthEventDescriptor);

        public:
            PartitionHealthEventDescriptor() = default;
            ~PartitionHealthEventDescriptor() = default;
            std::wstring GenerateReportDescriptionInternal(Health::HealthContext const & c) const override;
        };        
    }
}
