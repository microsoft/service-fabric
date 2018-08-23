// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class HealthEvaluationBase;
    typedef std::shared_ptr<HealthEvaluationBase> HealthEvaluationBaseSPtr;

    class HealthEvaluationBase
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(HealthEvaluationBase)

    public:
        HealthEvaluationBase();

        explicit HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND kind);
        HealthEvaluationBase(
            FABRIC_HEALTH_EVALUATION_KIND kind,
            FABRIC_HEALTH_STATE aggregatedHealthState);

        HealthEvaluationBase(HealthEvaluationBase && other) = default;
        HealthEvaluationBase & operator=(HealthEvaluationBase && other) = default;

        virtual ~HealthEvaluationBase();

        __declspec(property(get=get_Kind)) FABRIC_HEALTH_EVALUATION_KIND Kind;
        FABRIC_HEALTH_EVALUATION_KIND get_Kind() const { return kind_; }

        __declspec(property(get=get_Description)) std::wstring const & Description;
        std::wstring const & get_Description() const { return description_; }

        __declspec(property(get=get_AggregatedHealthState)) FABRIC_HEALTH_STATE AggregatedHealthState;
        FABRIC_HEALTH_STATE get_AggregatedHealthState() const { return aggregatedHealthState_; }

        virtual void SetDescription();
        virtual void GenerateDescription();

        // Generate a string description that represents the entire reason. Used by v2 Application health model.
        virtual std::wstring GetUnhealthyEvaluationDescription(std::wstring const & indent = L"") const;

        virtual Common::ErrorCode GenerateDescriptionAndTrimIfNeeded(
            size_t maxAllowedSize,
            __inout size_t & currentSize);

        virtual void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        virtual void WriteToEtw(uint16 contextSequenceId) const;
        std::wstring ToString() const;

        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;

        virtual Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        static int GetUnhealthyPercent(size_t unhealthyCount, ULONG totalCount);

        // Needed for serialization activation feature
        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static HealthEvaluationBase * CreateNew(FABRIC_HEALTH_EVALUATION_KIND kind);
        static HealthEvaluationBaseSPtr CreateSPtr(FABRIC_HEALTH_EVALUATION_KIND kind);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::Kind, kind_)
            SERIALIZABLE_PROPERTY(Constants::Description, description_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(HealthEvaluationBase, FABRIC_HEALTH_EVALUATION_KIND, kind_, CreateSPtr)

        // kind_ is always initialized in derived classes, do not add dynamic size
        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(aggregatedHealthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    protected:
        static Common::GlobalWString Delimiter;

        virtual Common::ErrorCode TryAdd(
            size_t maxAllowedSize,
            __inout size_t & currentSize);

    protected:
        FABRIC_HEALTH_EVALUATION_KIND kind_;
        std::wstring description_;
        FABRIC_HEALTH_STATE aggregatedHealthState_;
    };

    template<FABRIC_HEALTH_EVALUATION_KIND Kind>
    class HealthEvaluationSerializationTypeActivator
    {
    public:
        static HealthEvaluationBase * CreateNew()
        {
            return new HealthEvaluationBase(FABRIC_HEALTH_EVALUATION_KIND_INVALID);
        }

        static HealthEvaluationBaseSPtr CreateSPtr()
        {
            return make_shared<HealthEvaluationBase>(FABRIC_HEALTH_EVALUATION_KIND_INVALID);
        }
    };

    // template specializations, to be defined in derived classes
#define DEFINE_HEALTH_EVALUATION_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)                               \
    template<> class HealthEvaluationSerializationTypeActivator<IDL_ENUM>                             \
    {                                                                                                 \
    public:                                                                                           \
        static HealthEvaluationBase * CreateNew()                                                     \
        {                                                                                             \
            return new TYPE_SERVICEMODEL();                                                           \
        }                                                                                             \
        static HealthEvaluationBaseSPtr CreateSPtr()                                                  \
        {                                                                                             \
            return std::make_shared<TYPE_SERVICEMODEL>();                                             \
        }                                                                                             \
    };
}
