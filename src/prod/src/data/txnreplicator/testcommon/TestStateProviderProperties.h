// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        class TestStateProviderProperties :
            public Data::Utilities::FileProperties
        {
            K_FORCE_SHARED(TestStateProviderProperties)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public:
            __declspec(property(get = get_TestStateProviderInfoHandle, put = put_TestStateProviderInfoHandle)) Data::Utilities::BlockHandle::SPtr TestStateProviderInfoHandle;
            Data::Utilities::BlockHandle::SPtr get_TestStateProviderInfoHandle() const { return testStateProviderInfoHandle_; }
            void put_TestStateProviderInfoHandle(__in Data::Utilities::BlockHandle & blocksHandle) { testStateProviderInfoHandle_ = &blocksHandle; }

        public:
            void Write(__in Data::Utilities::BinaryWriter & writer) override;

            void ReadProperty(
                __in Data::Utilities::BinaryReader & reader,
                __in KStringView const & property,
                __in ULONG32) override;

        private:
            // Keys for the properties
            const KStringView TestStateProviderPropertyName_ = L"TestStateProviderInfo";

            Data::Utilities::BlockHandle::SPtr testStateProviderInfoHandle_;
        };
    }
}
