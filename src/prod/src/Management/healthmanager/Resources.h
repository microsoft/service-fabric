// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {

#define COMMON_RESOURCE( ResourceProperty, ResourceName ) \
    DEFINE_STRING_RESOURCE(CommonResource, ResourceProperty, COMMON, ResourceName) \

        class Resources
        {
            DECLARE_SINGLETON_RESOURCE(Resources)

            COMMON_RESOURCE(EntryTooLarge, ENTRY_TOO_LARGE)
            COMMON_RESOURCE(MaxResultsReached, Max_Results_Reached)

            HM_RESOURCE(QueryJobQueueFull, QueryJobQueueFull)
            HM_RESOURCE(ApplicationNotFound, ApplicationNotFound)
            HM_RESOURCE(ApplicationPolicyNotSet, ApplicationPolicyNotSet)
            HM_RESOURCE(ClusterPolicyNotSet, ClusterPolicyNotSet)
            HM_RESOURCE(ParentApplicationNotFound, ParentApplicationNotFound)
            HM_RESOURCE(ParentDeployedApplicationNotFound, ParentDeployedApplicationNotFound)
            HM_RESOURCE(ParentPartitionNotFound, ParentPartitionNotFound)
            HM_RESOURCE(ParentServiceNotFound, ParentServiceNotFound)
            HM_RESOURCE(ServiceTypeNotSet, ServiceTypeNotSet)
            HM_RESOURCE(ParentNodeNotFound, ParentNodeNotFound)
            HM_RESOURCE(InvalidQueryName, InvalidQueryName)
            HM_RESOURCE(MissingQueryParameter, MissingQueryParameter)
            HM_RESOURCE(InvalidQueryParameter, InvalidQueryParameter)
            HM_RESOURCE(EntityDeletedOrCleanedUp, EntityDeletedOrCleanedUp)
            HM_RESOURCE(NoHierarchySystemReport, NoHierarchySystemReport)
            HM_RESOURCE(NoSystemReport, NoSystemReport)
            HM_RESOURCE(DeleteEntityDueToParent, DeleteEntityDueToParent)
            HM_RESOURCE(EntityNotFound, EntityNotFound)
            HM_RESOURCE(SystemApplicationEvaluationError, SystemApplicationEvaluationError)
        };
    }
}
