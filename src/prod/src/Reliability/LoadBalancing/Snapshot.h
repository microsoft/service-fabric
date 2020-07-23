// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Node.h"
#include "Service.h"
#include "ServiceDomain.h"
#include "FailoverUnit.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Snapshot
        {
            DENY_COPY(Snapshot);

        public:
            Snapshot();
            Snapshot(Snapshot && other);
            Snapshot(std::map<std::wstring, ServiceDomain::DomainData> && serviceDomainSnapshot);

            __declspec (property(get=get_ServiceDomainSnapshot)) std::map<std::wstring, ServiceDomain::DomainData> const& ServiceDomainSnapshot;
            std::map<std::wstring, ServiceDomain::DomainData> const & get_ServiceDomainSnapshot() const { return serviceDomainSnapshot_; }

            __declspec (property(get=get_CreatedTimeUtc)) Common::DateTime CreatedTimeUtc;
            Common::DateTime get_CreatedTimeUtc() const { return createdTimeUtc_; }

            ServiceDomain::DomainData const* GetRGDomainData() const;
            void SetRGDomain(std::wstring const& rgDomain) { rgDomainId_ = rgDomain; }
        private:
            Common::DateTime createdTimeUtc_;
            std::map<std::wstring, ServiceDomain::DomainData> serviceDomainSnapshot_;
            std::wstring rgDomainId_;
        };
    }
}
