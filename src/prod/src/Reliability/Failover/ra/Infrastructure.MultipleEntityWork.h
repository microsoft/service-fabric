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
            // Represents work that spans multiple failover units
            // this object should be created and then jobs representing the work for each individual FT should be added
            // this should then be executed by the multiple ft work manager
            // It also supports a callback that can be invoked when all the FTs complete execution
            class MultipleEntityWork
            {
                DENY_COPY(MultipleEntityWork);

            public:

                typedef std::function<void(MultipleEntityWork &, ReconfigurationAgent &)> CompleteFunctionPtr;
                typedef std::function<EntityJobItemBaseSPtr(EntityEntryBaseSPtr const &, std::shared_ptr<MultipleEntityWork> const &)> FactoryFunctionPtr;

                MultipleEntityWork(std::wstring const & activityId, CompleteFunctionPtr completeFunction);

                MultipleEntityWork(std::wstring const & activityId);

                virtual ~MultipleEntityWork();

                __declspec(property(get = get_ActivityId)) std::wstring const & ActivityId;
                std::wstring const & get_ActivityId() const { return activityId_; }

                __declspec(property(get = get_OnCompleteCallback)) CompleteFunctionPtr const & OnCompleteCallback;
                CompleteFunctionPtr const & get_OnCompleteCallback() const { return completeFunction_; }

                __declspec(property(get = get_JobItems)) std::vector<EntityJobItemBaseSPtr> const & JobItems;
                std::vector<EntityJobItemBaseSPtr> const & get_JobItems() const { return jobItems_; }

                __declspec(property(get = get_HasCompletionCallback)) bool HasCompletionCallback;
                bool get_HasCompletionCallback() const { return completeFunction_ != nullptr; }

                __declspec(property(get = get_HasStarted)) bool HasStarted;
                bool get_HasStarted() const { return hasStarted_; }

                void AddFailoverUnitJob(EntityJobItemBaseSPtr const &);

                void AddFailoverUnitJob(EntityJobItemBaseSPtr &&);

                void OnExecutionStarted();

                bool OnFailoverUnitJobComplete();

                template<typename TInput>
                TInput const & GetInput() const
                {
                    MultipleFailoverUnitWorkWithInput<TInput> const * casted = dynamic_cast<MultipleFailoverUnitWorkWithInput<TInput> const *>(this);
                    ASSERT_IF(casted == nullptr, "Invalid cast");
                    return casted->Input;
                }

                template<typename TInput>
                TInput & GetInput() 
                {
                    MultipleFailoverUnitWorkWithInput<TInput> * casted = dynamic_cast<MultipleFailoverUnitWorkWithInput<TInput> *>(this);
                    ASSERT_IF(casted == nullptr, "Invalid cast");
                    return casted->Input;
                }

                Common::ErrorCode GetFirstError() const
                {
                    ASSERT_IF(completed_.load() != 0, "Must have completed");

                    for (auto const & ji : JobItems)
                    {
                        if (!ji->CommitResult.IsSuccess())
                        {
                            return ji->CommitResult;
                        }
                    }

                    return Common::ErrorCode::Success();
                }

                Infrastructure::EntityEntryBaseList GetEntities() const
                {
                    Infrastructure::EntityEntryBaseList rv(JobItems.size());

                    for (size_t i = 0; i < JobItems.size(); ++i)
                    {
                        rv[i] = JobItems[i]->EntrySPtr;
                    }

                    return rv;
                }

            private:

                std::wstring const activityId_;
                CompleteFunctionPtr completeFunction_;
                std::vector<EntityJobItemBaseSPtr> jobItems_;
                Common::atomic_uint64 completed_;
                bool hasStarted_;
            };

            template<typename TInput>
            class MultipleFailoverUnitWorkWithInput : public MultipleEntityWork
            {
                DENY_COPY(MultipleFailoverUnitWorkWithInput);

            public:
                MultipleFailoverUnitWorkWithInput(
                    std::wstring const & activityId,
                    CompleteFunctionPtr completeFunction,
                    TInput && input)
                    : MultipleEntityWork(activityId, completeFunction),
                    input_(std::move(input))
                {
                }

                MultipleFailoverUnitWorkWithInput(
                    std::wstring const & activityId,
                    TInput && input)
                    : MultipleEntityWork(activityId),
                    input_(std::move(input))
                {
                }

                __declspec(property(get = get_Input)) TInput const & Input;
                TInput const & get_Input() const { return input_; }
                TInput & get_Input() { return input_; }

            private:
                TInput input_;
            };
        }
    }
}
