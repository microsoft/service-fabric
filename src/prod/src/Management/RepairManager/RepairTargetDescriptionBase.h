// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairTargetDescriptionBase;
        typedef std::shared_ptr<RepairTargetDescriptionBase> RepairTargetDescriptionBaseSPtr;

        class RepairTargetDescriptionBase
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            RepairTargetDescriptionBase();
            explicit RepairTargetDescriptionBase(RepairTargetKind::Enum kind);
            RepairTargetDescriptionBase(RepairTargetDescriptionBase const &);
            RepairTargetDescriptionBase(RepairTargetDescriptionBase &&);

            __declspec(property(get = get_Kind)) RepairTargetKind::Enum const Kind;
            RepairTargetKind::Enum const get_Kind() const { return kind_; }

            virtual bool operator == (RepairTargetDescriptionBase const & other) const;
            bool operator != (RepairTargetDescriptionBase const & other) const;

            virtual Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TARGET_DESCRIPTION const &);
            virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_TARGET_DESCRIPTION &) const;

            virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            virtual bool IsValid() const;

            virtual RepairTargetDescriptionBaseSPtr Clone() const;

            // Needed for serialization activation feature
            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static RepairTargetDescriptionBase * CreateNew(FABRIC_REPAIR_TARGET_KIND kind);
            static RepairTargetDescriptionBaseSPtr CreateSPtr(FABRIC_REPAIR_TARGET_KIND kind);

            FABRIC_FIELDS_01(kind_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            JSON_TYPE_ACTIVATOR_METHOD(RepairTargetDescriptionBase, FABRIC_REPAIR_TARGET_KIND, kind_, CreateSPtr)

        protected:
            RepairTargetKind::Enum kind_;
        };

        template<FABRIC_REPAIR_TARGET_KIND Kind>
        class RepairTargetSerializationTypeActivator
        {
        public:
            static RepairTargetDescriptionBase * CreateNew()
            {
                return new RepairTargetDescriptionBase(RepairTargetKind::Invalid);
            }

            static RepairTargetDescriptionBaseSPtr CreateSPtr()
            {
                return make_shared<RepairTargetDescriptionBase>(RepairTargetKind::Invalid);
            }
        };

        // template specializations, to be defined in derived classes
        #define DEFINE_REPAIR_TARGET_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)     \
        template<> class RepairTargetSerializationTypeActivator<IDL_ENUM>       \
        {                                                                       \
        public:                                                                 \
            static RepairTargetDescriptionBase * CreateNew()                    \
            {                                                                   \
                return new TYPE_SERVICEMODEL();                                 \
            }                                                                   \
            static RepairTargetDescriptionBaseSPtr CreateSPtr()                 \
            {                                                                   \
                return std::make_shared<TYPE_SERVICEMODEL>();                   \
            }                                                                   \
        };                                                                      \

    }
}
