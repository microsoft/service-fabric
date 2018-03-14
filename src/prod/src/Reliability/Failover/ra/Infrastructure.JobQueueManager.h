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
            // This class contains all the job queue, work item scheduling/creation infrastructre
            class RAJobQueueManager 
            {
                DENY_COPY(RAJobQueueManager);
            public:
                typedef std::tuple<EntityEntryBaseSPtr, EntityJobItemBaseSPtr> EntryJobItemListElement;
                typedef std::vector<EntryJobItemListElement> EntryJobItemList;

                RAJobQueueManager(
                    ReconfigurationAgent & root);

                ~RAJobQueueManager();

                void ScheduleJobItem(EntityJobItemBaseSPtr &&);

                void ScheduleJobItem(EntityJobItemBaseSPtr const &);

                void CreateJobItemsAndStartMultipleFailoverUnitWork(
                    MultipleEntityWorkSPtr const & work,
                    Infrastructure::EntityEntryBaseList const & entries,
                    MultipleEntityWork::FactoryFunctionPtr factory);

                void AddJobItemsAndStartMultipleFailoverUnitWork(
                    MultipleEntityWorkSPtr const & work,
                    EntryJobItemList const & jobItems);
    
                void ExecuteMultipleFailoverUnitWork(MultipleEntityWorkSPtr &&);

                void ExecuteMultipleFailoverUnitWork(MultipleEntityWorkSPtr const &);

                void Close();

            private:
                MultipleEntityWorkManager multipleEntityWorkManager_;
                ReconfigurationAgent & ra_;
            };
        }
    }
}

