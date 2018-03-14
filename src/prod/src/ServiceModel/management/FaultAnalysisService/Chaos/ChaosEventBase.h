// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosEventBase
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosEventBase)

        public:
            ChaosEventBase();

            explicit ChaosEventBase(FABRIC_CHAOS_EVENT_KIND kind);
            ChaosEventBase(
                FABRIC_CHAOS_EVENT_KIND kind,
                Common::DateTime timeStampUtc);

            ChaosEventBase(ChaosEventBase && other);
            ChaosEventBase & operator = (ChaosEventBase && other);

            virtual ~ChaosEventBase();

            __declspec(property(get = get_Kind)) FABRIC_CHAOS_EVENT_KIND Kind;
            FABRIC_CHAOS_EVENT_KIND get_Kind() const { return kind_; }

            __declspec(property(get = get_TimeStamp)) Common::DateTime const & TimeStampUtc;
            Common::DateTime const & get_TimeStamp() const { return timeStampUtc_; }

            virtual Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_CHAOS_EVENT & publicChaosEvent) const;

            virtual Common::ErrorCode FromPublicApi(
                FABRIC_CHAOS_EVENT const & publicChaosEvent);

            // Needed for serialization activation feature
            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static ChaosEventBase * CreateNew(FABRIC_CHAOS_EVENT_KIND kind);
            static std::shared_ptr<ChaosEventBase> CreateSPtr(FABRIC_CHAOS_EVENT_KIND kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TimeStampUtc, timeStampUtc_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            JSON_TYPE_ACTIVATOR_METHOD(ChaosEventBase, FABRIC_CHAOS_EVENT_KIND, kind_, CreateSPtr)

        protected:
            FABRIC_CHAOS_EVENT_KIND kind_;
            Common::DateTime timeStampUtc_;
        };

        typedef std::shared_ptr<ChaosEventBase> ChaosEventBaseSPtr;

        template<FABRIC_CHAOS_EVENT_KIND Kind>
        class ChaosEventSerializationTypeActivator
        {
        public:
            static ChaosEventBase * CreateNew()
            {
                return new ChaosEventBase(FABRIC_CHAOS_EVENT_KIND_INVALID);
            }

            static ChaosEventBaseSPtr CreateSPtr()
            {
                return make_shared<ChaosEventBase>(FABRIC_CHAOS_EVENT_KIND_INVALID);
            }
        };

        // template specializations, to be defined in derived classes
        #define DEFINE_CHAOS_EVENT_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)                    \
        template<> class ChaosEventSerializationTypeActivator<IDL_ENUM>                       \
        {                                                                                                 \
        public:                                                                                           \
            static ChaosEventBase * CreateNew()                                               \
            {                                                                                             \
            return new TYPE_SERVICEMODEL();                                                           \
            }                                                                                             \
            static ChaosEventBaseSPtr CreateSPtr()                                                \
            {                                                                                             \
            return std::make_shared<TYPE_SERVICEMODEL>();                                             \
            }                                                                                             \
        };                                                                                                \

    }
}
