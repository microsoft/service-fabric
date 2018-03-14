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
            // Base class for a background work manager that operates on a set of failover units
            // FTs are added to the set by updating the corresponding SetMembershipFlag on the FT 
            // while the FT itself is Locked
            //
            // When the retry timer fires or when work is requested by a caller this class
            // will create a MultipleFailoverUnitWork representing the set of fts
            // and then ask the derived class to create a job item for each of these
            //
            // This will then be executed - i.e. all the FT JobItems will be scheduled by the scheduler
            // Once the MultipleFTWork is completed the OnComplete function will be called
            // The derived class should return true/false to confirm whether the retry is needed
            class MultipleEntityBackgroundWorkManager
            {
                DENY_COPY(MultipleEntityBackgroundWorkManager);

            public:

                class Parameters
                {
                    DENY_COPY(Parameters);

                public:
                    Parameters() :
                        RetryInterval(nullptr),
                        RA(nullptr),
                        PerformanceCounter(nullptr),
                        SetCollection(nullptr)
                    {
                    }

                    std::wstring Id;
                    std::wstring Name;
                    TimeSpanConfigEntry * MinIntervalBetweenWork;
                    TimeSpanConfigEntry * RetryInterval;
                    ReconfigurationAgent * RA;
                    MultipleEntityWork::FactoryFunctionPtr FactoryFunction;
                    MultipleEntityWork::CompleteFunctionPtr CompleteFunction;
                    EntitySetIdentifier SetIdentifier;
                    Common::PerformanceCounterData * PerformanceCounter;
                    Infrastructure::EntitySetCollection * SetCollection;
                };

                MultipleEntityBackgroundWorkManager(Parameters const & parameters);

                virtual ~MultipleEntityBackgroundWorkManager();

                __declspec(property(get = get_FTSet)) EntitySet & FTSet;
                EntitySet & get_FTSet() { return set_; }

                __declspec(property(get = get_BackgroundWorkManager)) BackgroundWorkManagerWithRetry & BackgroundWorkManager;
                BackgroundWorkManagerWithRetry & get_BackgroundWorkManager() { return bgmr_; }

                // Use this when requesting work in the context of a locked ft so that changes to the set are reflected in the queue
                // Request work directly from the background work manager when iterating over multiple failover units
                void Request(StateMachineActionQueue & queue);

                void Request(std::wstring const & activityId);

                void Close();

                // Public because of the inability to capture this in a constructor in a std::function
                void BGMCallback_Internal(std::wstring const &);

            private:
                void OnCompleteCallbackHelper(MultipleEntityWork & work, ReconfigurationAgent & ra);

                std::wstring const id_;
                EntitySet set_;

                MultipleEntityWork::CompleteFunctionPtr completeFunction_;
                MultipleEntityWork::FactoryFunctionPtr factoryFunction_;
                BackgroundWorkManagerWithRetry bgmr_;
                ReconfigurationAgent & ra_;

                class ProcessRequestStateMachineAction : public StateMachineAction
                {
                    DENY_COPY(ProcessRequestStateMachineAction);

                public:
                    ProcessRequestStateMachineAction(MultipleEntityBackgroundWorkManager & owner);

                    void OnPerformAction(
                        std::wstring const & activityId, 
                        Infrastructure::EntityEntryBaseSPtr const & entity,
                        ReconfigurationAgent & reconfigurationAgent);

                    void OnCancelAction(ReconfigurationAgent&);

                private:
                    MultipleEntityBackgroundWorkManager & owner_;
                };
            };
        }
    }
}

