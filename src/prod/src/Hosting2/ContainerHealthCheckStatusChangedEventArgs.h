// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerHealthCheckStatusChangedEventArgs : public Common::EventArgs
    {
    public:
        ContainerHealthCheckStatusChangedEventArgs(std::vector<ContainerHealthStatusInfo> && healthStatusInfoList)
            : healthStatusInfoList_(move(healthStatusInfoList))
        {
        }

        __declspec(property(get = get_HealthStatusInfoList)) std::vector<ContainerHealthStatusInfo> const & HealthStatusInfoList;
        std::vector<ContainerHealthStatusInfo> const & get_HealthStatusInfoList() const { return healthStatusInfoList_; }

    private:
        std::vector<ContainerHealthStatusInfo> healthStatusInfoList_;
    };
}
