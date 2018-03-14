// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace WeightType
    {
        enum Enum
        {
            Zero = 0,
            Low = 1,
            Medium = 2,
            High = 3,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        WeightType::Enum GetWeightType(std::wstring const & val);
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT ToPublicApi(__in WeightType::Enum const & weight);
        WeightType::Enum FromPublicApi(FABRIC_SERVICE_LOAD_METRIC_WEIGHT const &);
		std::wstring ToString(WeightType::Enum const & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Zero)
            ADD_ENUM_VALUE(Low)
            ADD_ENUM_VALUE(Medium)
            ADD_ENUM_VALUE(High)
        END_DECLARE_ENUM_SERIALIZER()

    };
}
