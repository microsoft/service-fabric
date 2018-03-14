// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace RollingUpgradeMode
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Invalid:
                w << L"Invalid";
                return;
            case UnmonitoredAuto:
                w << L"UnmonitoredAuto";
                return;
            case UnmonitoredManual:
                w << L"UnmonitoredManual";
                return;
            case Monitored:
                w << L"Monitored";
                return;
            default:
                w << wformatString("Unknown RollingUpgradeMode value {0}", static_cast<int>(val));
            }
        }

        ENUM_STRUCTURED_TRACE(RollingUpgradeMode, FirstValidEnum, LastValidEnum)
    }
}

ServiceModel::RollingUpgradeMode::Enum ServiceModel::RollingUpgradeMode::FromPublicApi(FABRIC_ROLLING_UPGRADE_MODE const & mode)
{
    switch (mode)
    {
    case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
        return ServiceModel::RollingUpgradeMode::UnmonitoredAuto;

    case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
        return ServiceModel::RollingUpgradeMode::UnmonitoredManual;

    case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
        return ServiceModel::RollingUpgradeMode::Monitored;

    default:
        return ServiceModel::RollingUpgradeMode::Invalid;
    }
}

FABRIC_ROLLING_UPGRADE_MODE ServiceModel::RollingUpgradeMode::ToPublicApi(ServiceModel::RollingUpgradeMode::Enum const & mode)
{
    switch (mode)
    {
    case ServiceModel::RollingUpgradeMode::UnmonitoredAuto:
        return FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;

    case ServiceModel::RollingUpgradeMode::UnmonitoredManual:
        return FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;

    case ServiceModel::RollingUpgradeMode::Monitored:
        return FABRIC_ROLLING_UPGRADE_MODE_MONITORED;

    default:
        return FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    }
}
