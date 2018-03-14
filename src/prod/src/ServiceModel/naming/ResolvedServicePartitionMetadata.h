// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolvedServicePartitionMetadata
        : public Common::IFabricJsonSerializable
    {
    public:
        ResolvedServicePartitionMetadata() {}

        ResolvedServicePartitionMetadata(          
            Common::Guid const &partitionId,
            __int64 fmVersion,
            __int64 storeVersion,
            Reliability::GenerationNumber const &generationNumber)
            : partitionId_(partitionId)
            , fmVersion_(fmVersion)
            , storeVersion_(storeVersion)
            , generationNumber_(generationNumber)
        {
        }

        __declspec(property(get=get_Cuid)) Common::Guid const & PartitionId;
        inline Common::Guid const & get_Cuid() const { return partitionId_; }

        __declspec(property(get=get_FMVersion)) __int64 FMVersion;
        inline __int64 get_FMVersion() const { return fmVersion_; }

        __declspec(property(get=get_Generation)) Reliability::GenerationNumber const & Generation;
        inline Reliability::GenerationNumber const & get_Generation() const { return generationNumber_; }

        __declspec(property(get=get_StoreVersion)) __int64 StoreVersion;
        inline __int64 get_StoreVersion() const { return storeVersion_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::FMVersion, fmVersion_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreVersion, storeVersion_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::GenerationNumber, generationNumber_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        Common::Guid partitionId_;
        __int64 fmVersion_;
        __int64 storeVersion_;
        Reliability::GenerationNumber generationNumber_;
    };

    typedef std::shared_ptr<ResolvedServicePartitionMetadata> ResolvedServicePartitionMetadataSPtr;
}
