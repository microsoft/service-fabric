// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("HealthEntityParent");

HealthEntityParent::HealthEntityParent(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    std::wstring const & childEntityId,
    HealthEntityKind::Enum parentEntityKind,
    GetParentFromCacheCallback const & getParentCallback)
    : childEntityId_(childEntityId)
    , partitionedReplicaId_(partitionedReplicaId)
    , lock_()
    , parentEntityKind_(parentEntityKind)
    , getParentCallback_(getParentCallback)
    , parent_()
    , parentAttributes_()
    , throttleNoParentTrace_(false)
{
}

HealthEntityParent::~HealthEntityParent()
{
}

AttributesStoreDataSPtr HealthEntityParent::get_Attributes() const
{
    AcquireReadLock lock(lock_);
    return parentAttributes_;
}

HealthEntitySPtr HealthEntityParent::GetLockedParent() const
{
    AcquireReadLock lock(lock_);
    return parent_.lock();
}

bool HealthEntityParent::get_HasSystemReport() const
{
    auto attributesCopy = this->Attributes;
    if (!attributesCopy)
    {
        return false;
    }

    if (attributesCopy->IsMarkedForDeletion || attributesCopy->IsCleanedUp)
    {
        return false;
    }

    return attributesCopy->HasSystemReport;
}

bool HealthEntityParent::IsSetAndUpToDate() const
{
    auto attributesCopy = this->Attributes;
    return attributesCopy && !attributesCopy->IsStale && !attributesCopy->IsCleanedUp;
}

Common::ErrorCode HealthEntityParent::HasAttributeMatch(
    std::wstring const & attributeName,
    std::wstring const & attributeValue,
    __out bool & hasMatch) const
{
    auto attributesCopy = this->Attributes;
    hasMatch = false;
    if (attributesCopy)
    {
        return attributesCopy->HasAttributeMatch(attributeName, attributeValue, /*out*/hasMatch);
    }

    // If the parent is not set, return error. The parent may exist and not be updated in the child entity
    return ErrorCode(ErrorCodeValue::HealthEntityNotFound);
}

void HealthEntityParent::Update(Common::ActivityId const & activityId)
{
    // Check whether is up to date using reader lock
    if (IsSetAndUpToDate())
    {
        return;
    }

    // If needs update, take writer lock
    AcquireWriteLock lock(lock_);
    auto parentEntity = parent_.lock();
    if (!parentEntity ||
        !parentAttributes_ ||
        (!parentAttributes_->IsStale && parentAttributes_->IsCleanedUp))
    {
        parentEntity = getParentCallback_();
        if (parentEntity)
        {
            // Get a copy of the attributes (this takes parent lock)
            parentAttributes_ = parentEntity->GetAttributesCopy();

            // Keep a weak pointer to the parent parentEntity to be able to update the attributes when they are marked as stale
            parent_ = parentEntity;

            throttleNoParentTrace_ = false;
            HMEvents::Trace->SetParent(
                childEntityId_,
                activityId,
                *parentAttributes_);
        }
        else if (parentAttributes_ && parentAttributes_->IsCleanedUp)
        {
            // If the parent attributes have been cleaned up and the parent is not in store anymore, keep the old parent.
            // This allows cleaning up the children because of deleted parent.
            throttleNoParentTrace_ = false;
            return;
        }
        else
        {
            // Update parent
            parentAttributes_.reset();
            parent_.reset();

            if (!throttleNoParentTrace_)
            {
                HMEvents::Trace->NoParent(
                    childEntityId_,
                    activityId,
                    parentEntityKind_);
                // Throttle next NoParent traces
                throttleNoParentTrace_ = true;
            }
        }
    }
    else if (parentAttributes_->IsStale)
    {
        // Update the attributes only.
        // This takes parent lock
        throttleNoParentTrace_ = false;
        parentAttributes_ = parentEntity->GetAttributesCopy();
        HMEvents::Trace->UpdateParent(
            childEntityId_,
            activityId,
            *parentAttributes_);
    }
}

// ================================================
// Node specific class
// ================================================
HealthEntityNodeParent::HealthEntityNodeParent(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    std::wstring const & childEntityId,
    GetParentFromCacheCallback const & getParentCallback)
    : HealthEntityParent(partitionedReplicaId, childEntityId, HealthEntityKind::Node, getParentCallback)
{
}

HealthEntityNodeParent::~HealthEntityNodeParent()
{
}

bool HealthEntityNodeParent::get_HasSystemReport() const
{
    auto attributesCopy = this->Attributes;
    if (!attributesCopy)
    {
        return false;
    }

    if (attributesCopy->IsMarkedForDeletion || attributesCopy->IsCleanedUp)
    {
        return false;
    }

    // If the node is marked as error by System.FM (NodeDown), then consider 
    // the hierarchy report missing
    if (attributesCopy->HasSystemError)
    {
        return false;
    }

    return attributesCopy->HasSystemReport;
}

template bool HealthEntityNodeParent::HasSameNodeInstance<ReplicaEntity>(AttributesStoreDataSPtr const & childAttributes) const;
template bool HealthEntityNodeParent::HasSameNodeInstance<DeployedApplicationEntity>(AttributesStoreDataSPtr const & childAttributes) const;
template bool HealthEntityNodeParent::HasSameNodeInstance<DeployedServicePackageEntity>(AttributesStoreDataSPtr const & childAttributes) const;

template <class TChildEntity>
bool HealthEntityNodeParent::HasSameNodeInstance(
    AttributesStoreDataSPtr const & childAttributes) const
{
    auto parentAttributes = this->Attributes;
    if (parentAttributes)
    {
        auto & castedChildAttributes = TChildEntity::GetCastedAttributes(childAttributes);
        auto & castedParentAttributes = NodeEntity::GetCastedAttributes(parentAttributes);

        return castedChildAttributes.AttributeSetFlags.IsNodeInstanceIdSet() &&
            castedChildAttributes.NodeInstanceId == castedParentAttributes.NodeInstanceId;
    }

    return false;
}

template bool HealthEntityNodeParent::ShouldDeleteChild<ReplicaEntity>(AttributesStoreDataSPtr const & childAttributes) const;
template bool HealthEntityNodeParent::ShouldDeleteChild<DeployedApplicationEntity>(AttributesStoreDataSPtr const & childAttributes) const;
template bool HealthEntityNodeParent::ShouldDeleteChild<DeployedServicePackageEntity>(AttributesStoreDataSPtr const & childAttributes) const;

template <class TChildEntity>
bool HealthEntityNodeParent::ShouldDeleteChild(
    AttributesStoreDataSPtr const & childAttributes) const
{
    ErrorCode error(ErrorCodeValue::Success);
    AttributesStoreDataSPtr attributes = this->Attributes;
    if (!attributes)
    {
        // If parent node is not set, do not delete, wait for it to become available
        return false;
    }

    auto const & childCastedAttributes = TChildEntity::GetCastedAttributes(childAttributes);
    auto & castedAttributes = NodeEntity::GetCastedAttributes(attributes);

    bool shouldDelete = false;
    if (childCastedAttributes.AttributeSetFlags.IsNodeInstanceIdSet() &&
        castedAttributes.NodeInstanceId != FABRIC_INVALID_NODE_INSTANCE_ID &&
        (castedAttributes.NodeInstanceId < childCastedAttributes.NodeInstanceId))
    {
        // The parent node is for an older instance, do not delete the newer child in this case
        shouldDelete = false;
    }
    else if (castedAttributes.IsMarkedForDeletion || castedAttributes.IsCleanedUp)
    {
        shouldDelete = true;
    }
    else if (castedAttributes.HasSystemError)
    {
        // If parent instance matches or is higher than replica node instance, mark entity should be deleted
        if (childCastedAttributes.AttributeSetFlags.IsNodeInstanceIdSet() &&
            (castedAttributes.NodeInstanceId >= childCastedAttributes.NodeInstanceId))
        {
            shouldDelete = true;
        }
    }
    else if (childCastedAttributes.AttributeSetFlags.IsNodeInstanceIdSet() &&
        (castedAttributes.NodeInstanceId > childCastedAttributes.NodeInstanceId))
    {
        // Parent is healthy, and its instance is higher than the one reported in Replica
        shouldDelete = true;
    }

    if (shouldDelete)
    {
        HMEvents::Trace->DeleteChild(
            childEntityId_,
            partitionedReplicaId_.TraceId,
            childCastedAttributes,
            castedAttributes);
    }

    return shouldDelete;
}
