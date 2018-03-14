// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairImpactDescriptionBase;
        typedef std::shared_ptr<RepairImpactDescriptionBase> RepairImpactDescriptionBaseSPtr;
        typedef std::shared_ptr<const RepairImpactDescriptionBase> RepairImpactDescriptionBaseConstSPtr;

        class RepairImpactDescriptionBase
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            RepairImpactDescriptionBase();
            explicit RepairImpactDescriptionBase(RepairImpactKind::Enum kind);
            RepairImpactDescriptionBase(RepairImpactDescriptionBase const &);
            RepairImpactDescriptionBase(RepairImpactDescriptionBase &&);

            __declspec(property(get=get_Kind)) RepairImpactKind::Enum const Kind;
            RepairImpactKind::Enum const get_Kind() const { return kind_; }

            virtual bool operator == (RepairImpactDescriptionBase const & other) const;
            bool operator != (RepairImpactDescriptionBase const & other) const;

            virtual Common::ErrorCode FromPublicApi(FABRIC_REPAIR_IMPACT_DESCRIPTION const &);
            virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_IMPACT_DESCRIPTION &) const;

            virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            virtual bool IsValid() const;
            virtual bool IsZeroImpact() const;

            virtual RepairImpactDescriptionBaseSPtr Clone() const;

            // Needed for serialization activation feature
            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static RepairImpactDescriptionBase * CreateNew(FABRIC_REPAIR_IMPACT_KIND kind);
            static RepairImpactDescriptionBaseSPtr CreateSPtr(FABRIC_REPAIR_IMPACT_KIND kind);

            FABRIC_FIELDS_01(kind_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            JSON_TYPE_ACTIVATOR_METHOD(RepairImpactDescriptionBase, FABRIC_REPAIR_IMPACT_KIND, kind_, CreateSPtr)

        protected:
            RepairImpactKind::Enum kind_;
        };

        template<FABRIC_REPAIR_IMPACT_KIND Kind>
        class RepairImpactSerializationTypeActivator
        {
        public:
            static RepairImpactDescriptionBase * CreateNew()
            {
                return new RepairImpactDescriptionBase(RepairImpactKind::Invalid);
            }

            static RepairImpactDescriptionBaseSPtr CreateSPtr()
            {
                return make_shared<RepairImpactDescriptionBase>(RepairImpactKind::Invalid);
            }
        };

        // template specializations, to be defined in derived classes
        #define DEFINE_REPAIR_IMPACT_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)     \
        template<> class RepairImpactSerializationTypeActivator<IDL_ENUM>       \
        {                                                                       \
        public:                                                                 \
            static RepairImpactDescriptionBase * CreateNew()                    \
            {                                                                   \
                return new TYPE_SERVICEMODEL();                                 \
            }                                                                   \
            static RepairImpactDescriptionBaseSPtr CreateSPtr()                 \
            {                                                                   \
                return std::make_shared<TYPE_SERVICEMODEL>();                   \
            }                                                                   \
        };                                                                      \

    }
}
