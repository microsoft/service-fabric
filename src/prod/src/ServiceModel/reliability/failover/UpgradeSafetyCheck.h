// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpgradeSafetyCheck;
    typedef std::shared_ptr<UpgradeSafetyCheck> UpgradeSafetyCheckSPtr;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class UpgradeSafetyCheckWrapper
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:

        UpgradeSafetyCheckWrapper();
        explicit UpgradeSafetyCheckWrapper(UpgradeSafetyCheckSPtr && safetyCheck);
        explicit UpgradeSafetyCheckWrapper(UpgradeSafetyCheckSPtr const& safetyCheck);

        UpgradeSafetyCheckWrapper(UpgradeSafetyCheckWrapper const& other);
        UpgradeSafetyCheckWrapper & operator=(UpgradeSafetyCheckWrapper const& other);

        UpgradeSafetyCheckWrapper(UpgradeSafetyCheckWrapper && other);
        UpgradeSafetyCheckWrapper & operator=(UpgradeSafetyCheckWrapper && other);

        __declspec(property(get = get_UpgradeSafetyCheck)) UpgradeSafetyCheckSPtr const& SafetyCheck;
        UpgradeSafetyCheckSPtr const& get_UpgradeSafetyCheck() const { return safetyCheck_; }

        bool operator==(UpgradeSafetyCheckWrapper const & other) const;
        bool operator!=(UpgradeSafetyCheckWrapper const & other) const;
        bool Equals(UpgradeSafetyCheckWrapper const & other, bool ignoreDynamicContent) const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const;
        Common::ErrorCode FromPublicApi(
            FABRIC_UPGRADE_SAFETY_CHECK const &);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SafetyCheck, safetyCheck_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_01(safetyCheck_);

    private:

        UpgradeSafetyCheckSPtr safetyCheck_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class UpgradeSafetyCheck
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:

        UpgradeSafetyCheck();
        UpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind);

        __declspec (property(get = get_Kind)) UpgradeSafetyCheckKind::Enum Kind;
        UpgradeSafetyCheckKind::Enum get_Kind() const { return kind_; }

        virtual bool Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const;

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_UPGRADE_SAFETY_CHECK const & safetyCheck);

        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static UpgradeSafetyCheckSPtr CreateSPtr(UpgradeSafetyCheckKind::Enum kind);

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES();
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_);
        END_JSON_SERIALIZABLE_PROPERTIES();

        JSON_TYPE_ACTIVATOR_METHOD(UpgradeSafetyCheck, UpgradeSafetyCheckKind::Enum, kind_, CreateSPtr);

        FABRIC_FIELDS_01(kind_);

    protected:

        UpgradeSafetyCheckKind::Enum kind_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class SeedNodeUpgradeSafetyCheck : public UpgradeSafetyCheck
    {
    public:

        SeedNodeUpgradeSafetyCheck();
        SeedNodeUpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind);

        virtual bool Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const override;

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const override;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_UPGRADE_SAFETY_CHECK const &) override;

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class PartitionUpgradeSafetyCheck : public UpgradeSafetyCheck
    {
    public:

        PartitionUpgradeSafetyCheck();
        PartitionUpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind, Common::Guid partitionId);

        __declspec (property(get = get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        virtual bool Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const override;

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const override;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_UPGRADE_SAFETY_CHECK const &) override;

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN();
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionId, partitionId_);
        END_JSON_SERIALIZABLE_PROPERTIES()

    FABRIC_FIELDS_01(partitionId_);

    private:

        Common::Guid partitionId_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::UpgradeSafetyCheckWrapper);
