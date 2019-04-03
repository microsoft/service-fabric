// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpApplicationGateway;

namespace HttpApplicationGateway
{
    namespace CertValidationPolicy
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case None: w << Constants::CertValidationNone; return;
            case ServiceCertificateThumbprints: w << Constants::CertValidationServiceCertificateThumbprints; return;
            case ServiceCommonNameAndIssuer: w << Constants::CertValidationServiceCommonNameAndIssuer; return;
            default:
                w.Write("{0:x}", (unsigned int)e);
            }
        }

        Common::ErrorCode TryParse(std::wstring const & stringInput, _Out_ Enum & certValidationPolicy)
        {
            certValidationPolicy = None;

            if (Common::StringUtility::AreEqualCaseInsensitive(stringInput, Constants::CertValidationNone))
            {
                return Common::ErrorCode();
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(stringInput, Constants::CertValidationServiceCertificateThumbprints))
            {
                certValidationPolicy = ServiceCertificateThumbprints;
                return Common::ErrorCode();
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(stringInput, Constants::CertValidationServiceCommonNameAndIssuer))
            {
                certValidationPolicy = ServiceCommonNameAndIssuer;
                return Common::ErrorCode();
            }

            return Common::ErrorCodeValue::InvalidArgument;
        }
    }

    void HttpApplicationGatewayConfig::Initialize()
    {
        CertValidationPolicy::Enum policyVal;
        auto error = CertValidationPolicy::TryParse(ApplicationCertificateValidationPolicy, policyVal);
        ASSERT_IFNOT(error.IsSuccess(), "Invalid policy: {0}", policyVal);
        CertValidationPolicyVal = policyVal;

        vector<wstring> headerList;
        // Headers in RemoveServiceResponseHeaders can be delimited by "," or ";" 
        // ";" is mainly added to support specifying multiple headers as the default 
        // Using , as delimiter in default value (Date,Header) breaks Configurations.csv parsing.
        // Lot of instances in our codebase split the lines in configurations.csv based on the comma delimiter only
        // and will also end up splitting comma inside quotes.
        // Adding parsing logic to split with ignoring delimiter inside quotes is complex since we don't have
        // clear picture of all the places that need to be changed. 
        StringUtility::Split<wstring>(RemoveServiceResponseHeaders, headerList, L",;", true);
        for (wstring const&header : headerList)
        {
            ResponseHeadersToRemoveMap.insert(pair<wstring, bool>(header, true));
        }
    }
}

DEFINE_SINGLETON_COMPONENT_CONFIG(HttpApplicationGatewayConfig)
