// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace PartitionSelectorType
        {
            enum Enum
            {
                None = 0,
                Singleton = 1,
                Named = 2,
                UniformInt64 = 3,
                PartitionId = 4,
                Random = 5,

                LAST_ENUM_PLUS_ONE = 6,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(PartitionSelectorType)

            Enum FromPublicApi(FABRIC_PARTITION_SELECTOR_TYPE);
            FABRIC_PARTITION_SELECTOR_TYPE ToPublicApi(Enum const);
        };
    }
}
