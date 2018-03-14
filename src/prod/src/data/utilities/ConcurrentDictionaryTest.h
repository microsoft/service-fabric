// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class ConcurrentDictionaryTest
        {
        public:
            Common::CommonConfig config; // load the config object as its needed for the tracing to work

            ConcurrentDictionaryTest()
            {
                NTSTATUS status;
                status = KtlSystem::Initialize(FALSE, &ktlSystem_);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                ktlSystem_->SetStrictAllocationChecks(TRUE);
            }

            ~ConcurrentDictionaryTest()
            {
                ktlSystem_->Shutdown();
            }

            KAllocator& GetAllocator()
            {
                return ktlSystem_->NonPagedAllocator();
            }

            KThreadPool& GetThreadPool()
            {
                return ktlSystem_->DefaultSystemThreadPool();
            }

            void RunDictionaryTest_Add1(int cLevel, int initSize, int threads, int addsPerThread);
            void RunDictionaryTest_Update1(int cLevel, int threads, int updatesPerThread);
            void RunDictionaryTest_Read1(int cLevel, int threads, int readsPerThread);
            void RunDictionaryTest_Remove1(int cLevel, int threads, int removesPerThread);
            void RunDictionaryTest_GetOrAdd(int cLevel, int initSize, int threads, int addsPerThread);

        private:
            KtlSystem* ktlSystem_;
        };

        template<typename T>
        class WorkItem : public KThreadPool::WorkItem
        {
        public:
            WorkItem(T const & work) : work_(work)
            {
            }

            virtual VOID Execute()
            {
                work_();
            }

        private:
            T work_;
        };
    }
}
