// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackageDescription
        {
            DENY_COPY_ASSIGNMENT(ServicePackageDescription);

        public:
            static Common::GlobalWString const FormatHeader;

            ServicePackageDescription(
                ServiceModel::ServicePackageIdentifier  && servicePackageIdentifier ,
                std::map<std::wstring, double> && requiredResources,
                std::vector<std::wstring> && containersImages);

            ServicePackageDescription(ServicePackageDescription const& other);

            ServicePackageDescription(ServicePackageDescription && other);

            ServicePackageDescription(ServicePackageDescription const& sp1, ServicePackageDescription const& sp2);

            ServicePackageDescription & operator = (ServicePackageDescription && other);

            bool operator == (ServicePackageDescription const& other) const;
            bool operator != (ServicePackageDescription const& other) const;

            __declspec(property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return servicePackageIdentifier_.ServicePackageName; }

            __declspec(property(get = get_ServicePackageIdentifier)) ServiceModel::ServicePackageIdentifier const& ServicePackageIdentifier;
            ServiceModel::ServicePackageIdentifier const& get_ServicePackageIdentifier() const { return servicePackageIdentifier_; }

            __declspec(property(get = get_RequiredResources)) std::map<std::wstring, double> const& RequiredResources;
            std::map<std::wstring, double> const& get_RequiredResources() const { return requiredResources_; }

            __declspec(property(get = get_IsGoverned)) bool IsGoverned;
            bool get_IsGoverned() const { return !requiredResources_.empty(); }

            __declspec(property(get = get_CorrectedRequiredResources)) std::map<std::wstring, uint> const& CorrectedRequiredResources;
            std::map<std::wstring, uint> const& get_CorrectedRequiredResources() const { return correctedRequiredResources_; }

            __declspec(property(get = get_ContainersImages)) std::vector<std::wstring> const& ContainersImages;
            std::vector<std::wstring> const& get_ContainersImages() const { return containersImages_; }

            uint GetMetricRgReservation(std::wstring const & metricName) const;

            bool IsAnyResourceReservationIncreased(ServicePackageDescription const& other);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
        private:
            void CorrectCpuMetric();

            ServiceModel::ServicePackageIdentifier servicePackageIdentifier_;

            //this maps each metric to a particular value that represents the load of this SP
            std::map<std::wstring, double> requiredResources_;

            // This map represents each metric in the way in which PLB sees it.
            std::map<std::wstring, uint> correctedRequiredResources_;

            // list of container images for the code packages of this service package
            std::vector<std::wstring> containersImages_;
        };
    }
}
