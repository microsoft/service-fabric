// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TestCommon
    {
        class TestRuntimeFolders
            : public KObject<TestRuntimeFolders>
            , public KShared<TestRuntimeFolders>
            , public TxnReplicator::IRuntimeFolders
        {
            K_FORCE_SHARED(TestRuntimeFolders)
            K_SHARED_INTERFACE_IMP(IRuntimeFolders)

        public:
            static TxnReplicator::IRuntimeFolders::SPtr Create(
                __in LPCWSTR workingDirectory,
                __in KAllocator & allocator);

            LPCWSTR get_WorkDirectory() const override;

        private: // Constructor
            TestRuntimeFolders(__in LPCWSTR workingDirectory);

            KString::SPtr workDirectory_;
        };
    }
}

