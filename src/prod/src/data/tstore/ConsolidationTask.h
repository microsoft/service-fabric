// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONSOLIDATION_TASK_TAG 'ctTG'

// TODO: Make this more general??
namespace Data
{
    namespace TStore
    {
        class ConsolidationTask
            : public KObject<ConsolidationTask>
            , public KShared<ConsolidationTask>
        {
            K_FORCE_SHARED(ConsolidationTask)
        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result);

            __declspec(property(get = get_Awaitable, put = set_Awaitable)) ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> & Awaitable;
            ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> & get_Awaitable()
            {
                return awaitable_;
            }

            void set_Awaitable(__in ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> && value)
            {
                awaitable_ = Ktl::Move(value);
            }

            ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> RunAsync();

        private:
            ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> awaitable_;
        };
    }
}
