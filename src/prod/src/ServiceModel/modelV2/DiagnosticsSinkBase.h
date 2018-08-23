// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class DiagnosticsSinkBase;
        typedef std::shared_ptr<DiagnosticsSinkBase> DiagnosticsSinkBaseSPtr;

        class DiagnosticsSinkBase
        : public ModelType
        {
        public:
            DiagnosticsSinkBase() = default;
            DiagnosticsSinkBase(std::wstring kind) : kind_(kind) {}

            DEFAULT_MOVE_ASSIGNMENT(DiagnosticsSinkBase)
            DEFAULT_MOVE_CONSTRUCTOR(DiagnosticsSinkBase)
            DEFAULT_COPY_ASSIGNMENT(DiagnosticsSinkBase)
            DEFAULT_COPY_CONSTRUCTOR(DiagnosticsSinkBase)

            __declspec(property(get=get_Name, put=put_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }
            void put_Name(std::wstring const & value) { name_ = value; }
            /*
            __declspec(property(get=get_Kind, put=put_Kind)) DiagnosticsSinkKind::Enum const & Kind;
            DiagnosticsSinkKind::Enum const & get_Kind() const { return kind_; }
            void put_Kind(DiagnosticsSinkKind::Enum const & value) { kind_ = value; }
            */
            __declspec(property(get=get_Kind, put=put_Kind)) std::wstring const & Kind;
            std::wstring const & get_Kind() const { return kind_; }
            void put_Kind(std::wstring const & value) { kind_ = value; }

            __declspec(property(get=get_Description, put=put_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; }
            void put_Description(std::wstring const & value) { description_ = value; }

            static DiagnosticsSinkBase * CreateNew(DiagnosticsSinkKind::Enum kind);
            static DiagnosticsSinkBaseSPtr CreateSPtr(DiagnosticsSinkKind::Enum kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                //SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
                SERIALIZABLE_PROPERTY(L"kind", kind_)
                SERIALIZABLE_PROPERTY(L"name", name_)
				SERIALIZABLE_PROPERTY(L"description", description_)

				SERIALIZABLE_PROPERTY(L"accountName", accountName_)
				SERIALIZABLE_PROPERTY(L"namespace", namespace_)
				SERIALIZABLE_PROPERTY(L"maConfigUrl", maConfigUrl_)
				SERIALIZABLE_PROPERTY(L"fluentdConfigUrl", fluentdConfigUrl_)
				SERIALIZABLE_PROPERTY(L"autoKeyConfigUrl", autoKeyConfigUrl_)
            END_JSON_SERIALIZABLE_PROPERTIES()

			//JSON_TYPE_ACTIVATOR_METHOD(DiagnosticsSinkBase, DiagnosticsSinkKind::Enum, kind_, CreateSPtr)

            FABRIC_FIELDS_08(name_, kind_, description_, accountName_, namespace_, maConfigUrl_, fluentdConfigUrl_, autoKeyConfigUrl_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(kind_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)

				DYNAMIC_SIZE_ESTIMATION_MEMBER(accountName_)
				DYNAMIC_SIZE_ESTIMATION_MEMBER(namespace_)
				DYNAMIC_SIZE_ESTIMATION_MEMBER(maConfigUrl_)
				DYNAMIC_SIZE_ESTIMATION_MEMBER(fluentdConfigUrl_)
				DYNAMIC_SIZE_ESTIMATION_MEMBER(autoKeyConfigUrl_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode TryValidate(std::wstring const &traceId) const override
            {
                (void)traceId;
                return Common::ErrorCodeValue::Success;
            }

        protected:
            std::wstring name_;
            //DiagnosticsSinkKind::Enum kind_;
            std::wstring kind_;
			std::wstring description_;

			std::wstring accountName_;
			std::wstring namespace_;
			std::wstring maConfigUrl_;
			std::wstring fluentdConfigUrl_;
			std::wstring autoKeyConfigUrl_;
        };


        template<DiagnosticsSinkKind::Enum Kind>
        class DiagnosticsSinkTypeActivator
        {
        public:
            static DiagnosticsSinkBase * CreateNew()
            {
                return new DiagnosticsSinkBase();
            }

            static DiagnosticsSinkBaseSPtr CreateSPtr()
            {
                return make_shared<DiagnosticsSinkBase>();
            }
        };

        #define DEFINE_DIAGNOSTICS_SINK_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM) \
        template<> class DiagnosticsSinkTypeActivator<IDL_ENUM>                \
        {                                                                      \
        public:                                                                \
            static DiagnosticsSinkBase * CreateNew()                           \
            {                                                                  \
                return new TYPE_SERVICEMODEL();                                \
            }                                                                  \
                                                                               \
            static DiagnosticsSinkBaseSPtr CreateSPtr()                        \
            {                                                                  \
                return std::make_shared<TYPE_SERVICEMODEL>();                  \
            }                                                                  \
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::DiagnosticsSinkBase);
