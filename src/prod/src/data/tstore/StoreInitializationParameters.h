// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STOREINITPARAM_TAG 'tpIS'

namespace Data
{
    namespace TStore
    {
        class StoreInitializationParameters
            : public Data::Utilities::FileProperties
        {
            K_FORCE_SHARED(StoreInitializationParameters)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __in StoreBehavior storeBehavior,
                __in bool allowReadableSecondary,
                __out StoreInitializationParameters::SPtr & result);

            //reserved for fileproperties.h
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out StoreInitializationParameters::SPtr & result);

            __declspec(property(get = get_StoreBehavior)) StoreBehavior StoreBehaviorProperty;
            StoreBehavior get_StoreBehavior() const
            {
                return storeBehavior_;
            }

            __declspec(property(get = get_AllowReadableSecondary)) bool AllowReadableSecondary;
            bool get_AllowReadableSecondary() const
            {
                return allowReadableSecondary_;
            }

            __declspec(property(get = get_Version, put= put_Version)) ULONG32 Version;
            ULONG32 get_Version() const
            {
                return version_;
            }
            
            void Write(__in Data::Utilities::BinaryWriter& writer) override;

            //
            // Read the given property value.
            //
            void ReadProperty(
                __in Data::Utilities::BinaryReader& reader,
                __in KStringView const& property,
                __in ULONG32 valueSize) override;

            Data::Utilities::OperationData::SPtr ToOperationData(__in KAllocator& allocator);

            static StoreInitializationParameters::SPtr FromOperationData(
                __in KAllocator & allocator,
                __in Data::Utilities::OperationData const & initializationContext);

        private:

            void put_Version(__in ULONG32 version)
            {
                version_ = version;
            }

            StoreInitializationParameters(
                __in StoreBehavior storeBehavior,
                __in bool allowReadableSecondary);

            static const ULONG32 SerializationVersion = 1;

            KStringView storeBehaviorPropertyName_ = L"behavior";
            KStringView allowReadableSecondaryPropertyName_ = L"readablesecondary";
            ULONG32 version_;
            StoreBehavior storeBehavior_;
            bool allowReadableSecondary_;
        };
    }
}
