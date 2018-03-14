// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void WeightType::WriteToTextWriter(__in TextWriter & w, WeightType::Enum const & val)
{
    switch (val)
    {
    case WeightType::Zero:
        w << L"Zero";
        return;
    case WeightType::Low:
        w << L"Low";
        return;
    case WeightType::Medium:
        w << L"Medium";
        return;
    case WeightType::High:
        w << L"High";
        return;
    default:
        Common::Assert::CodingError("Unknown WeightType value {0}", static_cast<int>(val));
    }
}

WeightType::Enum WeightType::GetWeightType(std::wstring const & val)
{
    if (StringUtility::AreEqualCaseInsensitive(val, L"Zero"))
    {
        return WeightType::Zero;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"Low"))
    {
        return WeightType::Low;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"Medium"))
    {
        return WeightType::Medium;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"High"))
    {
        return WeightType::High;
    }
    else
    {
        Common::Assert::CodingError("Unknown WeightType value {0}", val);
    }
}

FABRIC_SERVICE_LOAD_METRIC_WEIGHT WeightType::ToPublicApi(__in WeightType::Enum const & weight)
{
    switch (weight)
    {
    case WeightType::Zero:
        return FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO;
    case WeightType::Low:
        return FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW;
    case WeightType::Medium:
        return FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM;
    case WeightType::High:
        return FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH;
    default:
        Common::Assert::CodingError("Unknown WeightType value {0}", static_cast<int>(weight));
    }
}

WeightType::Enum WeightType::FromPublicApi(FABRIC_SERVICE_LOAD_METRIC_WEIGHT const & weight)
{
    switch (weight)
    {
    case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
        return WeightType::Zero;
    case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
        return WeightType::Low;
    case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
        return WeightType::Medium;
    case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
        return WeightType::High;
    default:
        Common::Assert::CodingError("Unknown WeightType value {0}", static_cast<int>(weight));
    }
}

wstring WeightType::ToString(WeightType::Enum const & val)
{
	switch (val)
	{
		case(WeightType::High):
			return L"High";
		case(WeightType::Medium) :
			return L"Medium";
		case(WeightType::Low) :
			return L"Low";
		default:
			return L"Zero";
	}
}
