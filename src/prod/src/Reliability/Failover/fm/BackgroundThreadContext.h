// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class BackgroundThreadContext
        {
            DENY_COPY(BackgroundThreadContext);

        public:
            __declspec (property(get=get_ContextId)) std::wstring const& ContextId;
            std::wstring const& get_ContextId() const { return contextId_; }

            virtual ~BackgroundThreadContext() {}

            virtual BackgroundThreadContextUPtr CreateNewContext() const = 0;

            __declspec(property(get = get_UnprocessedFailoverUnitCount)) size_t UnprocessedFailoverUnitCount;
            size_t get_UnprocessedFailoverUnitCount() const { return unprocessedFailoverUnits_.size(); }

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit) = 0;

            virtual void Merge(BackgroundThreadContext const & context) = 0;

            virtual bool ReadyToComplete() { return false; }

            virtual void Complete(FailoverManager & fm, bool isIterationCompleted, bool isEnumerationAborted) = 0;

            void TransferUnprocessedFailoverUnits(BackgroundThreadContext & orginal); 

            bool MergeUnprocessedFailoverUnits(std::set<FailoverUnitId> const & failoverUnitIds);

            void ClearUnprocessedFailoverUnits();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        protected:
            BackgroundThreadContext(std::wstring const & contextId);

        private:
            std::wstring contextId_;
            std::set<FailoverUnitId> unprocessedFailoverUnits_;
        };
    }
}
