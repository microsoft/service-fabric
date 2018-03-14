// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ServiceCorrelationScheme
    {
        enum Enum
        {
            Affinity = 0,
            AlignedAffinity = 1,
            NonAlignedAffinity = 2,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        ServiceCorrelationScheme::Enum GetScheme(std::wstring const & val);        
        FABRIC_SERVICE_CORRELATION_SCHEME ToPublicApi(__in ServiceCorrelationScheme::Enum const & scheme);
        ServiceCorrelationScheme::Enum FromPublicApi(FABRIC_SERVICE_CORRELATION_SCHEME const &);
    };
}
