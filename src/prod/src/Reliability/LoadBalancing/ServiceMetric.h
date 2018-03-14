// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Constants.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceMetric
        {
            DEFAULT_COPY_ASSIGNMENT(ServiceMetric)
        public:
            static Common::GlobalWString PrimaryCountName;
            static Common::GlobalWString SecondaryCountName;
            static Common::GlobalWString ReplicaCountName;
            static Common::GlobalWString InstanceCountName;
            static Common::GlobalWString CountName;

            enum BuiltInType
            {
                None,
                PrimaryCount,
                SecondaryCount,
                ReplicaCount,
                InstanceCount,
                Count
            };

            ServiceMetric(std::wstring && name, double weight, uint primaryDefaultLoad, uint secondaryDefaultLoad, bool isRGMetric = false);

            ServiceMetric(ServiceMetric const & other);
                
            ServiceMetric(ServiceMetric && other);

            ServiceMetric & operator = (ServiceMetric && other);

            bool operator == (ServiceMetric const& other) const;
            bool operator != (ServiceMetric const& other) const;

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_IsPrimaryCount)) bool IsPrimaryCount;
            bool get_IsPrimaryCount() const { return builtInType_ == PrimaryCount; }

            __declspec (property(get=get_IsSecondaryCount)) bool IsSecondaryCount;
            bool get_IsSecondaryCount() const { return builtInType_ == SecondaryCount; }

            __declspec (property(get=get_IsReplicaCount)) bool IsReplicaCount;
            bool get_IsReplicaCount() const { return builtInType_ == ReplicaCount; }

            __declspec (property(get=get_IsInstanceCount)) bool IsInstanceCount;
            bool get_IsInstanceCount() const { return builtInType_ == InstanceCount; }

            __declspec (property(get=get_IsCount)) bool IsCount;
            bool get_IsCount() const { return builtInType_ == Count; }

            __declspec (property(get = get_IsRGMetric)) bool IsRGMetric;
            bool get_IsRGMetric() const { return isRGMetric_; }

            __declspec (property(get=get_IsBuiltIn)) bool IsBuiltIn;
            bool get_IsBuiltIn() const { return builtInType_ != None; }

            __declspec (property(get=get_Weight)) double Weight;
            double get_Weight() const { return weight_; }

            __declspec (property(get=get_PrimaryDefaultLoad, put=put_PrimaryDefaultLoad)) uint PrimaryDefaultLoad;
            uint get_PrimaryDefaultLoad() const { return primaryDefaultLoad_; }
            void put_PrimaryDefaultLoad(uint primaryDefaultLoad) { primaryDefaultLoad_ = primaryDefaultLoad; }

            __declspec (property(get=get_SecondaryDefaultLoad, put = put_SecondaryDefaultLoad)) uint SecondaryDefaultLoad;
            uint get_SecondaryDefaultLoad() const { return secondaryDefaultLoad_; }
            void put_SecondaryDefaultLoad(uint secondaryDefaultLoad) { secondaryDefaultLoad_ = secondaryDefaultLoad; }

            __declspec (property(get=get_InstanceDefaultLoad)) uint InstanceDefaultLoad;
            uint get_InstanceDefaultLoad() const { return primaryDefaultLoad_; }

            // Check if the metric name contains the name of one of the default services
            // Does not guarantee absolute accuracy since users can specify their metric name similar to default service name
            bool IsMetricOfDefaultServices() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

        private:

            static BuiltInType ParseName(std::wstring const& name);

            std::wstring name_;
            BuiltInType builtInType_;
            double weight_;
            uint primaryDefaultLoad_;
            uint secondaryDefaultLoad_;
            bool isRGMetric_;
        };
    }
}
