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
            template <typename T>
            class HandlerParametersT
            {
            public:
                typedef LockedEntityPtr<T> LockedEntityPtrType;
                typedef typename EntityTraits<T>::EntityExecutionContextType EntityExecutionContextType;

                HandlerParametersT(
                    std::string const & traceId,
                    LockedEntityPtrType & entity,
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & actionQueue,
                    MultipleEntityWork const * work,
                    std::wstring const & activityId) : 
                    entity_(&entity),
                    ra_(&ra),
                    actionQueue_(&actionQueue),
                    activityId_(&activityId),
                    work_(work),
                    executionContext_(EntityTraits<T>::CreateEntityExecutionContext(ra, actionQueue, entity.UpdateContextObj, entity.Current)),
                    traceId_(&traceId)
                {
                }

            public:
                __declspec(property(get = get_ActivityId)) std::wstring const & ActivityId;
                std::wstring const & get_ActivityId() const { return *activityId_; }    

                __declspec(property(get = get_ActionQueue)) StateMachineActionQueue& ActionQueue;
                StateMachineActionQueue& get_ActionQueue() { return *actionQueue_; } 

                __declspec(property(get = get_RA)) ReconfigurationAgent & RA;
                ReconfigurationAgent & get_RA() { return *ra_; } 

                __declspec(property(get = get_Work)) MultipleEntityWork const * Work;
                MultipleEntityWork const * get_Work() const { return work_; }

                __declspec(property(get = get_Entity)) LockedEntityPtrType & Entity;
                LockedEntityPtrType & get_Entity() const { return *entity_; }

                __declspec(property(get = get_ExecutionContext)) EntityExecutionContextType & ExecutionContext;
                EntityExecutionContextType & get_ExecutionContext() { return executionContext_; }

                __declspec(property(get = get_TraceId)) std::string const &  TraceId;
                std::string const &  get_TraceId() const { return *traceId_; }

                void AssertHasWork() const
                {
                    ASSERT_IF(Work == nullptr, "FT must have work");
                }

            protected:
                LockedEntityPtrType * entity_;

            private:
                ReconfigurationAgent * ra_;
                StateMachineActionQueue * actionQueue_;
                std::wstring const * activityId_;
                MultipleEntityWork const * work_;
                EntityExecutionContextType executionContext_;
                std::string const * traceId_;
            };
        }
    }
}
