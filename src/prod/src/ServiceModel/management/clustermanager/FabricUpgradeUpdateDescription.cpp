// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("FabricUpgradeUpdateDescription");

FabricUpgradeUpdateDescription::FabricUpgradeUpdateDescription() 
    : upgradeType_(UpgradeType::Rolling)
    , updateDescription_()
    , healthPolicy_()
    , enableDeltaHealthEvaluation_()
    , upgradeHealthPolicy_()
    , applicationHealthPolicies_()
{ 
}

FabricUpgradeUpdateDescription::FabricUpgradeUpdateDescription(FabricUpgradeUpdateDescription && other) 
    : upgradeType_(move(other.upgradeType_))
    , updateDescription_(move(other.updateDescription_))
    , healthPolicy_(move(other.healthPolicy_))
    , enableDeltaHealthEvaluation_(move(other.enableDeltaHealthEvaluation_))
    , upgradeHealthPolicy_(move(other.upgradeHealthPolicy_))
    , applicationHealthPolicies_(move(other.applicationHealthPolicies_))
{ 
}

bool FabricUpgradeUpdateDescription::operator == (FabricUpgradeUpdateDescription const & other) const
{
    if (upgradeType_ != other.upgradeType_) { return false; }

    CHECK_EQUALS_IF_NON_NULL( updateDescription_ )

    CHECK_EQUALS_IF_NON_NULL( healthPolicy_ )

    CHECK_EQUALS_IF_NON_NULL( enableDeltaHealthEvaluation_ )

    CHECK_EQUALS_IF_NON_NULL( upgradeHealthPolicy_ )

    CHECK_EQUALS_IF_NON_NULL( applicationHealthPolicies_ )

    return true;
}

bool FabricUpgradeUpdateDescription::operator != (FabricUpgradeUpdateDescription const & other) const
{
    return !(*this == other);
}

void FabricUpgradeUpdateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w << "[ " << upgradeType_;
   
   if (updateDescription_) { w << ", " << *updateDescription_; }

   if (healthPolicy_) { w << ", " << *healthPolicy_; }

   if (enableDeltaHealthEvaluation_) { w << ", EnableDelta=" << *enableDeltaHealthEvaluation_; }

   if (upgradeHealthPolicy_) { w << ", " << *upgradeHealthPolicy_; }

   if (applicationHealthPolicies_ && applicationHealthPolicies_->size() > 0) { w << ", " << applicationHealthPolicies_->ToString(); }

   w << " ]";
}

bool FabricUpgradeUpdateDescription::HasUpdates() const
{
    return (this->UpdateDescription ||
            this->HealthPolicy ||
            this->EnableDeltaHealthEvaluation ||
            this->UpgradeHealthPolicy ||
            this->ApplicationHealthPolicies);
}

Common::ErrorCode FabricUpgradeUpdateDescription::FromPublicApi(
    FABRIC_UPGRADE_UPDATE_DESCRIPTION const & publicDescription)
{
    return ModifyUpgradeHelper::FromPublicUpdateDescription<
        FabricUpgradeUpdateDescription,
        FABRIC_UPGRADE_UPDATE_DESCRIPTION,
        ClusterHealthPolicy,
        FABRIC_CLUSTER_HEALTH_POLICY>(
            *this,
            publicDescription);
}
