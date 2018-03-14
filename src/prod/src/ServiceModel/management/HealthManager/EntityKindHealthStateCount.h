// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class EntityKindHealthStateCount
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
        public:
            EntityKindHealthStateCount();
            explicit EntityKindHealthStateCount(EntityKind::Enum entityKind);
            EntityKindHealthStateCount(
                EntityKind::Enum entityKind,
                HealthStateCount && healthCount);

            ~EntityKindHealthStateCount();

            EntityKindHealthStateCount(EntityKindHealthStateCount const & other);
            EntityKindHealthStateCount & operator=(EntityKindHealthStateCount const & other);

            EntityKindHealthStateCount(EntityKindHealthStateCount && other);
            EntityKindHealthStateCount & operator=(EntityKindHealthStateCount && other);

            __declspec(property(get = get_EntityKind)) EntityKind::Enum EntityKind;
            EntityKind::Enum get_EntityKind() const { return entityKind_; }

            __declspec(property(get = get_StateCount)) HealthStateCount const & StateCount;
            HealthStateCount const & get_StateCount() const { return healthCount_; }

            void AppendCount(HealthStateCount const & other) { return healthCount_.AppendCount(other); }

            void Add(FABRIC_HEALTH_STATE healthState) { return healthCount_.Add(healthState); }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT & publicHealthCount) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT const & publicHealthCount);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::EntityKind, entityKind_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthStateCount, healthCount_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(entityKind_, healthCount_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_ENUM_ESTIMATION_MEMBER(entityKind_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(healthCount_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            EntityKind::Enum entityKind_;
            HealthStateCount healthCount_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::HealthManager::EntityKindHealthStateCount)
