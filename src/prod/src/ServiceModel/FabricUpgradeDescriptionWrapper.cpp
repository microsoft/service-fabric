// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

FabricUpgradeDescriptionWrapper::FabricUpgradeDescriptionWrapper()
    : codeVersion_()
    , configVersion_()
    , upgradeType_(FABRIC_UPGRADE_KIND_INVALID)
    , rollingUpgradeMode_(FABRIC_ROLLING_UPGRADE_MODE_INVALID)
    , replicaSetCheckTimeoutInSeconds_(0)
    , forceRestart_(false)
    , monitoringPolicy_()
    , healthPolicy_()
    , enableDeltaHealthEvaluation_(false)
    , upgradeHealthPolicy_()
    , applicationHealthPolicies_()
{
}

ErrorCode FabricUpgradeDescriptionWrapper::FromPublicApi(FABRIC_UPGRADE_DESCRIPTION const & publicDescription)
{
    if (publicDescription.CodeVersion == NULL && publicDescription.ConfigVersion == NULL)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (publicDescription.CodeVersion != NULL)
    {
        HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.CodeVersion, false, codeVersion_);
        if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr); }
    }

    if (publicDescription.ConfigVersion != NULL)
    {
        HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.ConfigVersion, false, configVersion_);
        if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr); }
    }

    if (publicDescription.UpgradeKind == FABRIC_UPGRADE_KIND_ROLLING)
    {
        upgradeType_ = publicDescription.UpgradeKind;

        if (publicDescription.UpgradePolicyDescription == NULL)
        {
            return ErrorCodeValue::ArgumentNull;
        }

        auto policyDescription = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION *>(publicDescription.UpgradePolicyDescription);
        switch (policyDescription->RollingUpgradeMode)
        {
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                rollingUpgradeMode_ = policyDescription->RollingUpgradeMode;
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
                rollingUpgradeMode_ = policyDescription->RollingUpgradeMode;

                if (policyDescription->Reserved != NULL)
                {
                    auto policyDescriptionEx = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyDescription->Reserved);

                    if (policyDescriptionEx != NULL)
                    {
                        if (policyDescriptionEx->MonitoringPolicy != NULL)
                        {
                            monitoringPolicy_.FromPublicApi(*policyDescriptionEx->MonitoringPolicy);
                        }

                        if (policyDescriptionEx->HealthPolicy != NULL)
                        {
                            healthPolicy_ = make_shared<ClusterHealthPolicy>();
                            healthPolicy_->FromPublicApi(*reinterpret_cast<FABRIC_CLUSTER_HEALTH_POLICY*>(policyDescriptionEx->HealthPolicy));
                        }

                        auto policyDescriptionEx2 = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2*>(policyDescriptionEx->Reserved);

                        if (policyDescriptionEx2 != NULL)
                        {
                            if (policyDescriptionEx2->EnableDeltaHealthEvaluation == TRUE)
                            {
                                enableDeltaHealthEvaluation_ = true;
                            }
                            
                            if (policyDescriptionEx2->UpgradeHealthPolicy != NULL)
                            {
                                upgradeHealthPolicy_ = make_shared<ClusterUpgradeHealthPolicy>();
                                upgradeHealthPolicy_->FromPublicApi(*reinterpret_cast<FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY*>(policyDescriptionEx2->UpgradeHealthPolicy));
                            }

                            if (policyDescriptionEx2->Reserved != NULL)
                            {
                                auto policyDescriptionEx3 = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3*>(policyDescriptionEx2->Reserved);
                                if (policyDescriptionEx3->ApplicationHealthPolicyMap != NULL)
                                {
                                    applicationHealthPolicies_ = make_shared<ApplicationHealthPolicyMap>();
                                    auto error = applicationHealthPolicies_->FromPublicApi(*policyDescriptionEx3->ApplicationHealthPolicyMap);
                                    if (!error.IsSuccess())
                                    {
                                        return error;
                                    }
                                }
                            }
                        }
                    }
                }

                break;

            default:
                return ErrorCodeValue::InvalidArgument;
        }

        if (policyDescription->ForceRestart == TRUE)
        {
            forceRestart_ = true;
        }

        replicaSetCheckTimeoutInSeconds_ = policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds;
    }

    return ErrorCodeValue::Success;
}

void FabricUpgradeDescriptionWrapper::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_UPGRADE_DESCRIPTION & publicDescription) const
{
    publicDescription.CodeVersion = heap.AddString(codeVersion_);
    publicDescription.ConfigVersion = heap.AddString(configVersion_);

    if (upgradeType_ == FABRIC_UPGRADE_KIND_ROLLING)
    {
        publicDescription.UpgradeKind = upgradeType_;

        auto policyDescription = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION>();

        switch (rollingUpgradeMode_)
        {
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO:
            case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
                policyDescription->RollingUpgradeMode = rollingUpgradeMode_;
                break;

            case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
            {
                policyDescription->RollingUpgradeMode = rollingUpgradeMode_;

                auto policyDescriptionEx = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1>();

                auto monitoringPolicy = heap.AddItem<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY>();
                policyDescriptionEx->MonitoringPolicy = monitoringPolicy.GetRawPointer();
                monitoringPolicy_.ToPublicApi(heap, *monitoringPolicy);

                if (healthPolicy_)
                {
                    auto healthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
                    policyDescriptionEx->HealthPolicy = healthPolicy.GetRawPointer();
                    healthPolicy_->ToPublicApi(heap, *healthPolicy);
                }
                else
                {
                    policyDescriptionEx->HealthPolicy = NULL;
                }
                
                auto policyDescriptionEx2 = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2>();

                policyDescriptionEx2->EnableDeltaHealthEvaluation = enableDeltaHealthEvaluation_ ? TRUE : FALSE;

                if (upgradeHealthPolicy_)
                {
                    auto upgradeHealthPolicy = heap.AddItem<FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY>();
                    policyDescriptionEx2->UpgradeHealthPolicy = upgradeHealthPolicy.GetRawPointer();
                    upgradeHealthPolicy_->ToPublicApi(heap, *upgradeHealthPolicy);
                }

                auto policyDescriptionEx3 = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3>();

                if (applicationHealthPolicies_)
                {
                    auto publicAppHealthPolicyMap = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY_MAP>();
                    policyDescriptionEx3->ApplicationHealthPolicyMap = publicAppHealthPolicyMap.GetRawPointer();
                    applicationHealthPolicies_->ToPublicApi(heap, *publicAppHealthPolicyMap);
                }

                policyDescriptionEx2->Reserved = policyDescriptionEx3.GetRawPointer();

                policyDescriptionEx->Reserved = policyDescriptionEx2.GetRawPointer();

                policyDescription->Reserved = policyDescriptionEx.GetRawPointer();

                break;
            }

            default:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
        }

        policyDescription->ForceRestart = (forceRestart_ ? TRUE : FALSE);

        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds = replicaSetCheckTimeoutInSeconds_;

        publicDescription.UpgradePolicyDescription = policyDescription.GetRawPointer();
    }
    else
    {
        publicDescription.UpgradeKind = FABRIC_UPGRADE_KIND_INVALID;
        publicDescription.UpgradePolicyDescription = NULL;
    }
}

