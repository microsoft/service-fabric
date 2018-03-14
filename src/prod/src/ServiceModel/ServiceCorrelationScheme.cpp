// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ServiceCorrelationScheme::WriteToTextWriter(__in TextWriter & w, ServiceCorrelationScheme::Enum const & val)
{
    switch (val)
    {
    case ServiceCorrelationScheme::Affinity:
        w << L"Affinity";
        return;
    case ServiceCorrelationScheme::AlignedAffinity:
        w << L"AlignedAffinity";
        return;
    case ServiceCorrelationScheme::NonAlignedAffinity:
        w << L"NonAlignedAffinity";
        return;
    default:
        Common::Assert::CodingError("Unknown ServiceCorrelationScheme value {0}", static_cast<int>(val));
    }
}

ServiceCorrelationScheme::Enum ServiceCorrelationScheme::GetScheme(std::wstring const & val)
{
    if (StringUtility::AreEqualCaseInsensitive(val, L"Affinity"))
    {
        return ServiceCorrelationScheme::Affinity;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"AlignedAffinity"))
    {
        return ServiceCorrelationScheme::AlignedAffinity;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"NonAlignedAffinity"))
    {
        return ServiceCorrelationScheme::NonAlignedAffinity;
    }
    else
    {
        Common::Assert::CodingError("Unknown ServiceCorrelationScheme value {0}", val);
    }
}

FABRIC_SERVICE_CORRELATION_SCHEME ServiceCorrelationScheme::ToPublicApi(__in ServiceCorrelationScheme::Enum const & scheme)
{
    switch (scheme)
    {
    case ServiceCorrelationScheme::Affinity:
        return FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY;
    case ServiceCorrelationScheme::AlignedAffinity:
        return FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY;
    case ServiceCorrelationScheme::NonAlignedAffinity:
        return FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY;
    default:
        Common::Assert::CodingError("Unknown ServiceCorrelationScheme value {0}", static_cast<int>(scheme));
    }
}

ServiceCorrelationScheme::Enum ServiceCorrelationScheme::FromPublicApi(FABRIC_SERVICE_CORRELATION_SCHEME const & scheme)
{
    switch (scheme)
    {
    case FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
        return ServiceCorrelationScheme::Affinity;
    case FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
        return ServiceCorrelationScheme::AlignedAffinity;
    case FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
        return ServiceCorrelationScheme::NonAlignedAffinity;
    default:
        Common::Assert::CodingError("Unknown ServiceCorrelationScheme value {0}", static_cast<int>(scheme));
    }
}
