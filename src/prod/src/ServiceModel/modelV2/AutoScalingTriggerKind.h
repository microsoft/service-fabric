// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 
namespace ServiceModel
{
    namespace ModelV2
    {
        namespace AutoScalingTriggerKind
        {
            enum Enum
            {
                AverageLoad = 0
            };

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(AverageLoad)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}