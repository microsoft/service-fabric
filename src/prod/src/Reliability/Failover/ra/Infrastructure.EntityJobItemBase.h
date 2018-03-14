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
            class EntityJobItemBase 
            {
                DENY_COPY(EntityJobItemBase);
            protected:
                EntityJobItemBase(
                    EntityEntryBaseSPtr && entry,
                    std::wstring const & activityId,
                    JobItemCheck::Enum checks,
                    JobItemDescription const & description,
                    MultipleEntityWorkSPtr const &);

            public:
                virtual ~EntityJobItemBase();

                __declspec(property(get = get_EntrySPtr)) EntityEntryBaseSPtr EntrySPtr;
                EntityEntryBaseSPtr get_EntrySPtr() const { return entry_; }

                __declspec(property(get = get_ActivityId)) std::wstring const & ActivityId;
                std::wstring const & get_ActivityId() const { return activityId_; }

                __declspec(property(get = get_Work, put = set_Work)) MultipleEntityWorkSPtr const & Work;
                MultipleEntityWorkSPtr const & get_Work() const { return work_; }

                __declspec(property(get = get_TraceParameters)) JobItemDescription const & Description;
                JobItemDescription const & get_TraceParameters() const { return description_; }

                __declspec(property(get = get_CommitResult)) Common::ErrorCode const & CommitResult;
                Common::ErrorCode const & get_CommitResult() const { return commitResult_; }

                __declspec(property(get = get_Checks)) JobItemCheck::Enum Checks;
                JobItemCheck::Enum get_Checks() const { return checks_; }

                __declspec(property(get = get_HandlerReturnValue)) bool HandlerReturnValue;
                virtual bool get_HandlerReturnValue() const = 0;

                template<typename TJobItem>
                typename TJobItem::ContextType & GetContext()
                {
                    return As<TJobItem>().Context;
                }

                template<typename TJobItem>
                typename TJobItem::ContextType const & GetContext() const
                {
                    return As<TJobItem>().Context;
                }

                template<typename TJobItem>
                TJobItem & As()
                {
                    auto casted = dynamic_cast<TJobItem*>(this);
                    ASSERT_IF(casted == nullptr, "invalid cast");
                    return *casted;
                }

                template<typename TJobItem>
                TJobItem const & As() const
                {
                    auto casted = dynamic_cast<TJobItem const *>(this);
                    ASSERT_IF(casted == nullptr, "invalid cast");
                    return *casted;
                }

           protected:
                JobItemDescription const & description_;
                EntityEntryBaseSPtr const entry_;
                std::wstring const activityId_;
                MultipleEntityWorkSPtr work_;

                bool isStarted_;
                bool isCommitted_;
                bool wasDropped_;

                Common::ErrorCode commitResult_;

            private:
                JobItemCheck::Enum const checks_;
            };
        }
    }
}

