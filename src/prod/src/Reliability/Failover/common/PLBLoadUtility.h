// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class PLBLoadUtility
    {
    public:

        PLBLoadUtility()
        {}

        static bool IsMetricDefined(ServiceDescription const& serviceDescription, std::wstring const& metricName)
        {
            for (ServiceLoadMetricDescription const& defaultMetric : serviceDescription.Metrics)
            {
                if (defaultMetric.Name == metricName){
                    return true;
                }
            }

            return false;
        }

    private:

    };
}

