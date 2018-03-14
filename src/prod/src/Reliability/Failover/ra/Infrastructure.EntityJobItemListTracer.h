// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            /*
                This class handles tracing when a set of job items are executed for an entity
            */
            template<typename T>
            class EntityJobItemListTracer
            {
                DENY_COPY(EntityJobItemListTracer);

            public:
                typedef LockedEntityPtr<T> LockedEntityPtrType;
                typedef typename EntityTraits<T>::IdType IdType;
                typedef EntityJobItemBaseT<T> EntityJobItemBaseTType;

                EntityJobItemListTracer() {}

                void Trace(
                    EntityEntryBase const & entity,
                    IdType const & id,
                    LockedEntityPtrType & lockedEntity,
                    EntityJobItemList const & jobItems,
                    Common::ErrorCode const & commitResult,
                    Diagnostics::ScheduleEntityPerformanceData const & schedulePerfData,
                    Diagnostics::ExecuteEntityJobItemListPerformanceData const & executePerfData,
                    Diagnostics::CommitEntityPerformanceData const & commitPerfData,
                    ReconfigurationAgent & ra)
                {
                    std::vector<EntityJobItemTraceInformation> traceInfo;

                    for (auto const & it : jobItems)
                    {
                        static_cast<EntityJobItemBaseTType&>(*it).Trace(entity.GetEntityIdForTrace(), traceInfo, ra);
                    }

                    if (lockedEntity.IsEntityDeleted)
                    {
                        RAEventSource::Events->EntityDeleted(
                            entity.GetEntityIdForTrace(),
                            ra.NodeInstance,
                            commitResult,
                            traceInfo);

                        return;
                    }

                    if (lockedEntity && (lockedEntity.IsUpdating || !traceInfo.empty()))
                    {
                        EntityTraits<T>::Trace(id, ra.NodeInstance, *lockedEntity, traceInfo, commitResult, schedulePerfData, executePerfData, commitPerfData);
                    }
                }
            };
        }
    }
}



