// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Constants
        {
        public:
            // Trace Sources
            static Common::StringLiteral const DummyPLBSource;

            static Common::GlobalWString const MoveCostMetricName;
            
            static Common::GlobalWString const FaultDomainIdScheme;
            static Common::GlobalWString const UpgradeDomainIdScheme;

            static double const MetricWeightHigh;
            static double const MetricWeightMedium;
            static double const MetricWeightLow;
            static double const MetricWeightZero;

            static double const SmallValue;
            //implicit node properties
            static Common::GlobalWString const ImplicitNodeType;
            static Common::GlobalWString const ImplicitNodeName;
            static Common::GlobalWString const ImplicitUpgradeDomainId;
            static Common::GlobalWString const ImplicitFaultDomainId;

            static std::vector<Common::GlobalWString> const DefaultServicesNameList;
        };
    }
};
