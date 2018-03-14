// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "ServiceTypeDescription.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceType
        {
            DENY_COPY(ServiceType);

        public:
            explicit ServiceType(ServiceTypeDescription && desc);

            ServiceType(ServiceType && other);

            __declspec (property(get=get_ServiceTypeDescription)) ServiceTypeDescription const& ServiceTypeDesc;
            ServiceTypeDescription const& get_ServiceTypeDescription() const { return serviceTypeDesc_; }

            bool UpdateDescription(ServiceTypeDescription && desc);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            ServiceTypeDescription serviceTypeDesc_;
        };
    }
}
