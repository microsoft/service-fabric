// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TEST_TAG 'gtST'

namespace TStoreTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::TStore;
    using namespace Data::Utilities;

    template <typename T>
    class TestStateSerializer :
        public KObject<TestStateSerializer<T>>
        , public KShared<TestStateSerializer<T>>
        , public Data::StateManager::IStateSerializer<T>
    {
		K_FORCE_SHARED(TestStateSerializer)
        K_SHARED_INTERFACE_IMP(IStateSerializer)

    public:

		static NTSTATUS Create(
			__in KAllocator& allocator,
			__out SPtr& result)
		{
			NTSTATUS status;
			SPtr output = _new(TEST_TAG, allocator) TestStateSerializer();

			if (output == nullptr)
			{
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			status = output->Status();
			if (!NT_SUCCESS(status))
			{
				return status;
			}

			result = Ktl::Move(output);
			return STATUS_SUCCESS;
		}

        void Write(
            __in T value,
            __in BinaryWriter& binaryWriter)
        {
            binaryWriter.Write(value);
        }

        T Read(__in BinaryReader& binaryReader)
        {
            T value;
            binaryReader.Read(value);
            return value;
        }

        //T Read(
        //    __in T& baseValue,
        //    __in Data::Utilities::BinaryReader& binaryReader)
        //{
        //    //todo: implement if test needs
        //    return baseValue;
        //}

        //void Write(
        //    __in T& baseValue,
        //    __in T& targetValue,
        //    __in Data::Utilities::BinaryWriter& binaryWriter)
        //{
        //    //todo: implement if test needs
        //}
    };

	template<typename T>
	TestStateSerializer<T>::TestStateSerializer()
	{
	}

	template<typename T>
	TestStateSerializer<T>::~TestStateSerializer()
	{
	}
}
