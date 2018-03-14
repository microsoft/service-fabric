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
            template<typename T, typename TContext>
            class EntityJobItem : public EntityJobItemBaseT<T>
            {
                DENY_COPY(EntityJobItem);
            public:
                typedef TContext ContextType;
                typedef typename EntityTraits<T>::HandlerParametersType HandlerParametersType;

                typedef std::function<bool(HandlerParametersType &, TContext &)> TryProcessFunctionPtr;

                typedef typename EntityTraits<T>::DataType DataType;
                typedef EntityEntry<T> EntityEntryType;
                typedef std::shared_ptr<EntityEntryType> EntityEntrySPtrType;
                typedef typename EntityTraits<T>::IdType IdType;
                typedef LockedEntityPtr<T> LockedEntityPtrType;

                class Parameters
                {
                    DENY_COPY(Parameters);

                public:
                    Parameters(
                        EntityEntryBaseSPtr const & entry,
                        std::wstring const & activityId,
                        TryProcessFunctionPtr handler,
                        JobItemCheck::Enum checks,
                        JobItemDescription const & description)
                        : Entry(entry),
                        ActivityId(activityId),
                        Handler(handler),
                        Checks(checks),
                        Description(&description)
                    {
                    }

                    Parameters(
                        EntityEntryBaseSPtr const & entry,
                        std::wstring const & activityId,
                        TryProcessFunctionPtr handler,
                        JobItemCheck::Enum checks,
                        JobItemDescription const & description,
                        TContext && context)
                        : Entry(entry),
                        ActivityId(activityId),
                        Handler(handler),
                        Checks(checks),
                        Description(&description),
                        Context(std::move(context))
                    {
                    }

                    Parameters(
                        EntityEntryBaseSPtr const & entry,
                        MultipleEntityWorkSPtr const & work,
                        TryProcessFunctionPtr handler,
                        JobItemCheck::Enum checks,
                        JobItemDescription const & description)
                        : Entry(entry),
                        Work(work),
                        Handler(handler),
                        Checks(checks),
                        Description(&description)
                    {
                    }

                    Parameters(
                        EntityEntryBaseSPtr const & entry,
                        MultipleEntityWorkSPtr const & work,
                        TryProcessFunctionPtr handler,
                        JobItemCheck::Enum checks,
                        JobItemDescription const & description,
                        TContext && context)
                        : Entry(entry),
                        Work(work),
                        Handler(handler),
                        Checks(checks),
                        Description(&description),
                        Context(std::move(context))
                    {
                    }

                    Parameters(
                        EntityEntryBaseSPtr const & entry,
                        std::wstring && activityId,
                        TryProcessFunctionPtr handler,
                        JobItemCheck::Enum checks,
                        JobItemDescription const & traceParameters,
                        TContext && context)
                        : Entry(entry),
                        ActivityId(std::move(activityId)),
                        Handler(handler),
                        Checks(checks),
                        Description(&traceParameters),
                        Context(std::move(context))
                    {
                    }

                    Parameters(Parameters && other)
                        : Entry(std::move(other.Entry)),
                        ActivityId(std::move(other.ActivityId)),
                        Handler(std::move(other.Handler)),
                        Checks(std::move(other.Checks)),
                        Description(std::move(other.Description)),
                        Context(std::move(other.Context))
                    {
                    }

                    EntityEntryBaseSPtr Entry;
                    std::wstring ActivityId;
                    TryProcessFunctionPtr Handler;
                    JobItemCheck::Enum Checks;
                    JobItemDescription const * Description;
                    TContext Context;
                    MultipleEntityWorkSPtr Work;
                };

                EntityJobItem(Parameters && parameters)
                    : EntityJobItemBaseT<T>(
                    std::move(parameters.Entry),
                    parameters.Work != nullptr ? parameters.Work->ActivityId : parameters.ActivityId,
                    parameters.Checks,
                    *parameters.Description,
                    parameters.Work),
                    handler_(parameters.Handler),
                    context_(std::move(parameters.Context)),
                    handlerReturnValue_(false)
                {
                    ASSERT_IF(handler_ == nullptr, "Handler must be supplied");
                }

                __declspec(property(get = get_Id)) IdType const & Id;
                IdType const & get_Id() const { return GetId(); }

                bool get_HandlerReturnValue() const override { return handlerReturnValue_; }

                __declspec(property(get = get_Context)) TContext & Context;
                TContext & get_Context() { return context_; }
                TContext const & get_Context() const { return context_; }

                void Process(LockedEntityPtrType & lock, ReconfigurationAgent & ra) override
                {
                    ProcessInternal(lock, ra);
                }

                void FinishProcess(
                    LockedEntityPtrType & lock,
                    Infrastructure::EntityEntryBaseSPtr const & entry,
                    Common::ErrorCode const & commitResult,
                    ReconfigurationAgent & ra) override
                {
                    FinishProcessInternal(lock, entry, commitResult, ra);            
                }

                void Trace(
                    std::string const & traceId,
                    std::vector<EntityJobItemTraceInformation> & jobItemTraceInfos,
                    ReconfigurationAgent & ra) const override
                {
                    if (!ShouldTrace())
                    {
                        return;
                    }

                    TraceState<TContext::HasCorrelatedTrace>(traceId, jobItemTraceInfos, ra);
                }

            private:
                void ProcessInternal(
                    LockedEntityPtrType & lockedEntity,
                    ReconfigurationAgent & ra)
                {
                    ASSERT_IF(this->isStarted_, "Cannot call start twice");
                    ASSERT_IF(!lockedEntity.IsValid, "Lock must be valid");

                    this->isStarted_ = true;

                    if (lockedEntity.IsEntityDeleted)
                    {
                        this->wasDropped_ = true;
                        return;
                    }

                    if (lockedEntity.get_Current() != nullptr)
                    {
                        // Until there is a way to indicate if a write lock requires a commit
                        // Mark it as always requiring a commit
                        // The reason for this is earlier since there were only write locks
                        // the ft would never be copied
                        // now, the write lock copies the ft but if there is no enable update
                        // in memory changes are also lost
                        lockedEntity.EnableInMemoryUpdate();
                    }

                    if (!PerformJobItemChecks(lockedEntity, ra))
                    {
                        this->wasDropped_ = true;
                        return;
                    }

                    auto handlerParameters = EntityTraits<T>::CreateHandlerParameters(
                        this->entry_->GetEntityIdForTrace(), 
                        lockedEntity, 
                        ra, 
                        actionQueue_, 
                        this->work_.get(), 
                        this->ActivityId);
                    
                    handlerReturnValue_ = handler_(handlerParameters, Context);
                    
                    AssertInvariants(lockedEntity, handlerParameters);

                    this->wasDropped_ = !handlerReturnValue_;
                }

                void FinishProcessInternal(
                    LockedEntityPtrType &,
                    Infrastructure::EntityEntryBaseSPtr const & entry,
                    Common::ErrorCode const & commitResult,
                    ReconfigurationAgent & ra)
                {
                    ASSERT_IF(!this->isStarted_, "Must have been started");
                    
                    FailFastIfNeeded(entry, commitResult, ra);

                    this->commitResult_ = commitResult;

                    /*
                        Think carefully before moving this code out
                        There is a dependency between the entity being locked and certain actions
                        The AddToSet action is not safe to perform if the entity is unlocked and other
                        actions are allowed.

                        This needs to be fixed before entity actions can be truly performed outside entity lock

                        Consider
                        Thread T1 Locks entity
                        Thread T1 adds an action to add to set
                        Thread T1 finishes commit so state is updated
                        Thread T1 releases lock
                        Thread T2 locks entity
                        Thread T2 removes from set
                        Thread T2 finishes commit so state is updated
                        Thread T2 executes actions which remove the item from the set
                        Thread T1 executes actions which add the entity to the set
                        Entity must check in it's callback if the flag is still set

                        Entity is in the set
                        Thread T1 Locks entity
                        Thread T1 adds an action to remove to set
                        Thread T1 finishes commit so state is updated
                        Thread T1 releases lock
                        Thread T2 locks entity
                        Thread T2 adds an action to add to set
                        Thread T2 finishes commit
                        Thread T2 executes actions which add the item from the set
                        Thread T1 executes action to remove from the set
                    */
                    if (this->wasDropped_ || !commitResult.IsSuccess())
                    {
                        CancelActions(ra);
                    }
                    else
                    {
                        ExecuteActions(entry, ra);
                    }
                }

                void FailFastIfNeeded(
                    Infrastructure::EntityEntryBaseSPtr const & entry,
                    Common::ErrorCode const & commitResult,
                    ReconfigurationAgent & ra) const
                {
                    /*
                        The default policy on commit failure is to discard the commit and continue execution
                        This policy does not work for all job items
                        Some job items require successful commits as they are not retried 
                        For these job items fail fast on commit failure is the current approach
                        In case the commit fails because the RA is closing then do not FailFast
                    */
                    if (!commitResult.IsSuccess() && 
                        !commitResult.IsError(Common::ErrorCodeValue::RAStoreNotUsable) && 
                        !commitResult.IsError(Common::ErrorCodeValue::ObjectClosed) &&
                        !commitResult.IsError(Common::ErrorCodeValue::NotPrimary) && // This is specifically needed for the local store on v2 native stack
                        this->Description.ShouldFailFastOnCommitFailure)
                    {
                        auto msg = Common::wformatString("Failfast as commit for {0} {1} failed with error {2}", entry->GetEntityIdForTrace(), this->activityId_, commitResult);
                        ra.ProcessTerminationService.ExitProcess(msg);
                    }
                }

                void AssertInvariants(
                    LockedEntityPtrType & lockedEntity,
                    HandlerParametersType & handerParameters)
                {
                    if (lockedEntity.IsEntityDeleted || lockedEntity.Current == nullptr)
                    {
                        return;
                    }

                    EntityTraits<T>::AssertInvariants(*lockedEntity, handerParameters.ExecutionContext);
                }

                bool PerformJobItemChecks(
                    LockedEntityPtrType & lockedEntity,
                    ReconfigurationAgent & ra) const
                {
                    if (this->Checks & JobItemCheck::RAIsOpen)
                    {
                        if (!ra.IsOpen)
                        {
                            return false;
                        }
                    }

                    if (this->Checks & JobItemCheck::RAIsOpenOrClosing)
                    {
                        if (!ra.StateObj.IsOpenOrClosing)
                        {
                            return false;
                        }
                    }

                    return EntityTraits<T>::PerformChecks(this->Checks, lockedEntity.Current);
                }

                void CancelActions(ReconfigurationAgent & ra)
                {
                    actionQueue_.AbandonAllActions(ra);
                }

                void ExecuteActions(
                    EntityEntryBaseSPtr const & entry, 
                    ReconfigurationAgent & ra)
                {
                    actionQueue_.ExecuteAllActions(this->activityId_, entry, ra);
                }

                bool ShouldTrace() const
                {
                    switch (this->description_.Frequency)
                    {
                    case JobItemTraceFrequency::Always:
                        return true;

                    case JobItemTraceFrequency::Never:
                        return false;

                    case JobItemTraceFrequency::OnSuccessfulProcess:
                        return !this->wasDropped_;

                    default:
                        Common::Assert::CodingError("unknown enum value {0}", static_cast<int>(this->Description.Frequency));
                    }
                }

                template<bool hasCorrelatedTrace>
                void TraceState(
                    std::string const & ,
                    std::vector<EntityJobItemTraceInformation> & ,
                    ReconfigurationAgent & ) const
                {
                    static_assert(false, "HasCorrelatedTrace must be defined on context");
                }

                template<>
                void TraceState<true>(
                    std::string const & ,
                    std::vector<EntityJobItemTraceInformation> & jobItemTraceInfos,
                    ReconfigurationAgent & ra) const
                {
                    auto const & contextMetadata = GetContextMetadataForTrace<TContext::HasTraceMetadata>();
                    jobItemTraceInfos.push_back(EntityJobItemTraceInformation::CreateWithMetadata(this->activityId_, contextMetadata));
                    Context.Trace(this->entry_->GetEntityIdForTrace(), ra.NodeInstance, this->activityId_);
                }

                template<>
                void TraceState<false>(
                    std::string const & ,
                    std::vector<EntityJobItemTraceInformation> & jobItemTraceInfos,
                    ReconfigurationAgent & ) const
                {
                    jobItemTraceInfos.push_back(EntityJobItemTraceInformation::CreateWithName(this->activityId_, this->description_.Name));
                }

                template<bool hasMetadata>
                std::wstring const & GetContextMetadataForTrace() const
                {
                    static_assert(false, "HasTraceMetadata must be defined on context that has correlated trace");
                }

                template<>
                std::wstring const & GetContextMetadataForTrace<true>() const
                {
                    return Context.GetTraceMetadata();
                }

                template<>
                std::wstring const & GetContextMetadataForTrace<false>() const
                {
                    static std::wstring Empty;
                    return Empty;
                }

                IdType const & GetId() const
                {
                    // Needed in the node up ack processor etc
                    return this->entry_->As<EntityEntry<T>>().Id;
                }

                // TODO: Use a common action queue in the entity executor async operation
                // This will allow actions to be coalesced
                StateMachineActionQueue actionQueue_;

                bool handlerReturnValue_;

                TryProcessFunctionPtr handler_;
                TContext context_;
            };
        }
    }
}
