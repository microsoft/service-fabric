// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "ReplicaRole.h"
#include "LoadEntry.h"
#include "NodeSet.h"
#include "ServiceEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackageEntry
        {
            DENY_COPY(ServicePackageEntry);

        public:
            static Common::GlobalWString const TraceDescription;

            ServicePackageEntry(
                std::wstring const& name,
                LoadEntry && loads,
                std::vector<std::wstring> && images);

            ServicePackageEntry(ServicePackageEntry && other);

            ServicePackageEntry & operator = (ServicePackageEntry && other);

            __declspec(property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; };

            __declspec(property(get = get_Load)) LoadEntry const& Loads;
            LoadEntry const& get_Load() const { return loads_; }

            __declspec(property(get = get_ContainerImages)) std::vector<std::wstring> const& ContainerImages;
            std::vector<std::wstring> const& get_ContainerImages() const { return containerImages_; }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring name_;
            LoadEntry loads_;
            std::vector<std::wstring> containerImages_;
        };
    }
}
