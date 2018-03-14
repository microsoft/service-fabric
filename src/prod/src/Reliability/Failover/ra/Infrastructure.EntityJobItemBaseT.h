// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template <typename T>
            class EntityJobItemBaseT : public EntityJobItemBase
            {
                DENY_COPY(EntityJobItemBaseT);
            protected:
                typedef LockedEntityPtr<T> LockedEntityPtrType;

                EntityJobItemBaseT(
                    EntityEntryBaseSPtr && entry,
                    std::wstring const & activityId,
                    JobItemCheck::Enum checks,
                    JobItemDescription const & traceParameters,
                    MultipleEntityWorkSPtr const & work) :
                    EntityJobItemBase(std::move(entry), std::move(activityId), checks, traceParameters, work)
                {
                }

            public:
                // Trace out the actions and add information about whether trace is required by this job item or not
                virtual void Trace(
                    std::string const & traceId,
                    std::vector<EntityJobItemTraceInformation> &,
                    ReconfigurationAgent & ra) const = 0;

                virtual void Process(
                    LockedEntityPtrType & lock,
                    ReconfigurationAgent & ra) = 0;

                virtual void FinishProcess(
                    LockedEntityPtrType & lock,
                    Infrastructure::EntityEntryBaseSPtr const & entry,
                    Common::ErrorCode const & commitResult,
                    ReconfigurationAgent & ra) = 0;

                MultipleEntityWorkSPtr ReleaseWork()
                {
                    auto copy = std::move(work_);
                    return copy;
                }
            };
        }
    }
}

