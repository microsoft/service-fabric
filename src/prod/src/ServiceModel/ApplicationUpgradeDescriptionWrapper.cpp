// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationUpgradeDescriptionWrapper::ApplicationUpgradeDescriptionWrapper()
    : applicationName_()
    , targetAppTypeVersion_()
    , parameterList_()
    , upgradeType_(UpgradeType::Invalid)
    , rollingUpgradeMode_(RollingUpgradeMode::Invalid)
    , replicaSetCheckTimeoutInSeconds_(0)
    , forceRestart_(false)
    , monitoringPolicy_()
    , healthPolicy_()
{
}

Common::ErrorCode ApplicationUpgradeDescriptionWrapper::FromPublicApi(__in FABRIC_APPLICATION_UPGRADE_DESCRIPTION const &publicDesc)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDesc.ApplicationName, false, applicationName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(publicDesc.TargetApplicationTypeVersion, false, targetAppTypeVersion_);
    if (!error.IsSuccess()) { return error; }

    if ((publicDesc.UpgradeKind != FABRIC_APPLICATION_UPGRADE_KIND_ROLLING) || 
        (publicDesc.UpgradePolicyDescription == NULL))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    upgradeType_ = UpgradeType::Rolling;

    if (publicDesc.ApplicationParameters != NULL)
    {
        size_t maxParamLength = ServiceModelConfig::GetConfig().MaxApplicationParameterLength;
        for (ULONG i = 0; i < publicDesc.ApplicationParameters->Count; i++)
        {
            FABRIC_APPLICATION_PARAMETER parameter = publicDesc.ApplicationParameters->Items[i];
            wstring name;
            error = StringUtility::LpcwstrToWstring2(parameter.Name, false, name);
            if (!error.IsSuccess()) { return error; }
        
            wstring value;
            error = StringUtility::LpcwstrToWstring2(parameter.Value, true, 0, maxParamLength, value);
            if (!error.IsSuccess()) { return error; }

            parameterList_[name] = value;
        }
    }

    auto policyDescription = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION *>(publicDesc.UpgradePolicyDescription);

    switch (policyDescription->RollingUpgradeMode)
    {
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredAuto;
            break;

        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredManual;
            break;

        case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
            rollingUpgradeMode_ = RollingUpgradeMode::Monitored;

            if (policyDescription->Reserved != NULL)
            {
                auto policyDescriptionEx = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription->Reserved);

                if (policyDescriptionEx != NULL)
                {
                    if (policyDescriptionEx->MonitoringPolicy != NULL)
                    {
                        error = monitoringPolicy_.FromPublicApi(*(policyDescriptionEx->MonitoringPolicy));
                        if (!error.IsSuccess()) { return error; }
                    }

                    if (policyDescriptionEx->HealthPolicy != NULL)
                    {
                        healthPolicy_ = make_shared<ApplicationHealthPolicy>();
                        auto healthPolicy = reinterpret_cast<FABRIC_APPLICATION_HEALTH_POLICY*>(policyDescriptionEx->HealthPolicy);
                        error = healthPolicy_->FromPublicApi(*healthPolicy);
                        if (!error.IsSuccess()) { return error; }
                    }
                }
            }
            break;

        default:
            return ErrorCodeValue::InvalidArgument;
    }

    forceRestart_ = (policyDescription->ForceRestart == TRUE) ? true : false;
    replicaSetCheckTimeoutInSeconds_ = policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds;

    return ErrorCodeValue::Success;
}

void ApplicationUpgradeDescriptionWrapper::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_UPGRADE_DESCRIPTION & publicDescription) const
{
    publicDescription.ApplicationName = heap.AddString(applicationName_);
    publicDescription.TargetApplicationTypeVersion = heap.AddString(targetAppTypeVersion_);

    if (!parameterList_.empty())
    {
        auto parameterCollection = heap.AddItem<FABRIC_APPLICATION_PARAMETER_LIST>();
        publicDescription.ApplicationParameters = parameterCollection.GetRawPointer();
        parameterCollection->Count = static_cast<ULONG>(parameterList_.size());

        auto parameters = heap.AddArray<FABRIC_APPLICATION_PARAMETER>(parameterCollection->Count);
        parameterCollection->Items = &parameters[0];

        int i = 0;
        for (auto itr = parameterList_.begin(); itr != parameterList_.end(); ++itr, ++i)
        {
            parameters[i].Name = heap.AddString(itr->first);
            parameters[i].Value = heap.AddString(itr->second);
        }
    }
    else
    {
        publicDescription.ApplicationParameters = NULL;
    }

    switch (upgradeType_)
    {
        case UpgradeType::Rolling:
        case UpgradeType::Rolling_NotificationOnly:
        case UpgradeType::Rolling_ForceRestart:
            publicDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
            break;
        default:
            publicDescription.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_INVALID;
    }

    if (publicDescription.UpgradeKind == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
    {
        auto policyDescription = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION>();

        switch (rollingUpgradeMode_)
        {
            case RollingUpgradeMode::UnmonitoredAuto:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
                break;

            case RollingUpgradeMode::UnmonitoredManual:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
                break;

            case RollingUpgradeMode::Monitored:
            {
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;

                auto policyDescriptionEx = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1>();

                auto monitoringPolicy = heap.AddItem<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY>();
                policyDescriptionEx->MonitoringPolicy = monitoringPolicy.GetRawPointer();
                monitoringPolicy_.ToPublicApi(heap, *monitoringPolicy);

                if (healthPolicy_)
                {
                    auto healthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
                    policyDescriptionEx->HealthPolicy = healthPolicy.GetRawPointer();
                    healthPolicy_->ToPublicApi(heap, *healthPolicy);
                }
                else
                {
                    policyDescriptionEx->HealthPolicy = NULL;
                }

                policyDescription->Reserved = policyDescriptionEx.GetRawPointer();

                break;
            }

            default:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
        }

        policyDescription->ForceRestart = forceRestart_? TRUE : FALSE;

        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds = replicaSetCheckTimeoutInSeconds_;

        publicDescription.UpgradePolicyDescription = policyDescription.GetRawPointer();
    }
    else
    {
        publicDescription.UpgradePolicyDescription = NULL;
    }
}
