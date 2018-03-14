// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairScopeIdentifierBase;
        typedef std::shared_ptr<RepairScopeIdentifierBase> RepairScopeIdentifierBaseSPtr;
        typedef std::shared_ptr<const RepairScopeIdentifierBase> RepairScopeIdentifierBaseConstSPtr;

        class RepairScopeIdentifierBase
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            RepairScopeIdentifierBase();
            explicit RepairScopeIdentifierBase(RepairScopeIdentifierKind::Enum kind);
            RepairScopeIdentifierBase(RepairScopeIdentifierBase const &);
            RepairScopeIdentifierBase(RepairScopeIdentifierBase &&);

            __declspec(property(get = get_Kind)) RepairScopeIdentifierKind::Enum const Kind;
            RepairScopeIdentifierKind::Enum const get_Kind() const { return kind_; }

            virtual bool operator == (RepairScopeIdentifierBase const & other) const;
            bool operator != (RepairScopeIdentifierBase const & other) const;

            virtual bool IsValid() const { return false; }
            virtual bool IsVisibleTo(RepairScopeIdentifierBase const &) const { return false; }

            virtual Common::ErrorCode FromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER const &);
            virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_SCOPE_IDENTIFIER &) const;

            virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            // Needed for serialization activation feature
            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static RepairScopeIdentifierBase * CreateNew(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND kind);
            static RepairScopeIdentifierBaseSPtr CreateSPtr(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND kind);

            static Common::ErrorCode CreateSPtrFromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER const &, RepairScopeIdentifierBaseSPtr &);

            FABRIC_FIELDS_01(kind_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            JSON_TYPE_ACTIVATOR_METHOD(RepairScopeIdentifierBase, FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND, kind_, CreateSPtr)

        protected:
            RepairScopeIdentifierKind::Enum kind_;
        };

        template<FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND Kind>
        class RepairScopeIdentifierSerializationTypeActivator
        {
        public:
            static RepairScopeIdentifierBase * CreateNew()
            {
                return new RepairScopeIdentifierBase(RepairTargetKind::Invalid);
            }

            static RepairScopeIdentifierBaseSPtr CreateSPtr()
            {
                return make_shared<RepairScopeIdentifierBase>(RepairTargetKind::Invalid);
            }
        };

        // template specializations, to be defined in derived classes
        #define DEFINE_REPAIR_SCOPE_IDENTIFIER_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)     \
        template<> class RepairScopeIdentifierSerializationTypeActivator<IDL_ENUM>       \
        {                                                                       \
        public:                                                                 \
            static RepairScopeIdentifierBase * CreateNew()                   \
            {                                                                   \
                return new TYPE_SERVICEMODEL();                                 \
            }                                                                   \
            static RepairScopeIdentifierBaseSPtr CreateSPtr()                \
            {                                                                   \
                return std::make_shared<TYPE_SERVICEMODEL>();                   \
            }                                                                   \
        };                                                                      \

    }
}
