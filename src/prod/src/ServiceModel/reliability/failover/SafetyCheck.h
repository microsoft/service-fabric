// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class SafetyCheck;
    typedef std::shared_ptr<SafetyCheck> SafetyCheckSPtr;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class SafetyCheckWrapper
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
    public:

        SafetyCheckWrapper();
        explicit SafetyCheckWrapper(SafetyCheckSPtr && safetyCheck);
        explicit SafetyCheckWrapper(SafetyCheckSPtr const& safetyCheck);

        SafetyCheckWrapper(SafetyCheckWrapper const& other);
        SafetyCheckWrapper & operator=(SafetyCheckWrapper const& other);

        SafetyCheckWrapper(SafetyCheckWrapper && other);
        SafetyCheckWrapper & operator=(SafetyCheckWrapper && other);

        __declspec(property(get = get_SafetyCheck)) SafetyCheckSPtr const& SafetyCheck;
        SafetyCheckSPtr const& get_SafetyCheck() const { return safetyCheck_; }

        bool operator==(SafetyCheckWrapper const & other) const;
        bool operator!=(SafetyCheckWrapper const & other) const;
        bool Equals(SafetyCheckWrapper const & other, bool ignoreDynamicContent) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SAFETY_CHECK & safetyCheck) const;
        Common::ErrorCode FromPublicApi(
            FABRIC_SAFETY_CHECK const &);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SafetyCheck, safetyCheck_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(safetyCheck_);
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_01(safetyCheck_);

    private:

        SafetyCheckSPtr safetyCheck_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class SafetyCheck
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
    public:

        SafetyCheck();
        SafetyCheck(SafetyCheckKind::Enum kind);

        __declspec (property(get = get_Kind)) SafetyCheckKind::Enum Kind;
        SafetyCheckKind::Enum get_Kind() const { return kind_; }

        virtual bool Equals(SafetyCheck const & other, bool ignoreDynamicContent) const;

        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SAFETY_CHECK & safetyCheck) const;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SAFETY_CHECK const & safetyCheck);

        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static SafetyCheckSPtr CreateSPtr(SafetyCheckKind::Enum kind);

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES();
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_);
        END_JSON_SERIALIZABLE_PROPERTIES();

        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(kind_);
        END_DYNAMIC_SIZE_ESTIMATION()

        JSON_TYPE_ACTIVATOR_METHOD(SafetyCheck, SafetyCheckKind::Enum, kind_, CreateSPtr);

        FABRIC_FIELDS_01(kind_);

    protected:

        SafetyCheckKind::Enum kind_;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class SeedNodeSafetyCheck : public SafetyCheck
    {
    public:

        SeedNodeSafetyCheck();
        SeedNodeSafetyCheck(SafetyCheckKind::Enum kind);

        virtual bool Equals(SafetyCheck const & other, bool ignoreDynamicContent) const override;

        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SAFETY_CHECK & safetyCheck) const override;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SAFETY_CHECK const &) override;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN();
        END_DYNAMIC_SIZE_ESTIMATION()
            
        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class PartitionSafetyCheck : public SafetyCheck
    {
    public:

        PartitionSafetyCheck();
        PartitionSafetyCheck(SafetyCheckKind::Enum kind, Common::Guid partitionId);

        __declspec (property(get = get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        virtual bool Equals(SafetyCheck const & other, bool ignoreDynamicContent) const override;

        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SAFETY_CHECK & safetyCheck) const override;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SAFETY_CHECK const &) override;

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN();
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionId, partitionId_);
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN();
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_);
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_01(partitionId_);

    private:

        Common::Guid partitionId_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::SafetyCheckWrapper);
