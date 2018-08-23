// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Reliability
{
    namespace ScalingTriggerKind
    {
        enum Enum
        {
            Invalid = 0,
            AveragePartitionLoad = 1,
            AverageServiceLoad = 2
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(AveragePartitionLoad)
            ADD_ENUM_VALUE(AverageServiceLoad)
        END_DECLARE_ENUM_SERIALIZER()
    };
}
