//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class MigrationInitData : public Serialization::FabricSerializable
    {
    public:
        MigrationInitData();

        Common::ErrorCode FromBytes(std::vector<byte> const &);
        Common::ErrorCode ToBytes(__out std::vector<byte> &);
        
        __declspec (property(get=get_TargetReplicaSetSize)) int TargetReplicaSetSize;
        int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

        __declspec (property(get=get_MinReplicaSetSize)) int MinReplicaSetSize;
        int get_MinReplicaSetSize() const { return minReplicaSetSize_; }

        __declspec (property(get=get_Settings)) std::unique_ptr<MigrationSettings> const & Settings;
        std::unique_ptr<MigrationSettings> const & get_Settings() const { return settings_; }

        void Initialize(MigrationSettings const &);
        void Initialize(int targetReplicaSetSize, int minReplicaSetSize);
        void SetPhase(Common::Guid, MigrationPhase::Enum);
        MigrationPhase::Enum GetPhase(Common::Guid);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(targetReplicaSetSize_, minReplicaSetSize_, partitionPhaseMap_, settings_);

    public:

        class PartitionInitData : public Serialization::FabricSerializable
        {
            DENY_COPY( PartitionInitData )

        public:
            PartitionInitData();

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(TargetPhase);

        public:
            MigrationPhase::Enum TargetPhase;
        };

    private:

        int targetReplicaSetSize_;
        int minReplicaSetSize_;
        std::map<Common::Guid, std::shared_ptr<PartitionInitData>> partitionPhaseMap_;
        std::unique_ptr<MigrationSettings> settings_;
    };
}

DEFINE_USER_MAP_UTILITY(Common::Guid, std::shared_ptr<Store::MigrationInitData::PartitionInitData>)
