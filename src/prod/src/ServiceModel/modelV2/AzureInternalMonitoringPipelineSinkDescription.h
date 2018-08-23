// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class AzureInternalMonitoringPipelineSink
        : public DiagnosticsSinkBase
        {
        public:
            AzureInternalMonitoringPipelineSink() = default;

            DEFAULT_MOVE_ASSIGNMENT(AzureInternalMonitoringPipelineSink)
            DEFAULT_MOVE_CONSTRUCTOR(AzureInternalMonitoringPipelineSink)
            DEFAULT_COPY_ASSIGNMENT(AzureInternalMonitoringPipelineSink)
            DEFAULT_COPY_CONSTRUCTOR(AzureInternalMonitoringPipelineSink)

            __declspec(property(get=get_AccountName, put=put_AccountName)) std::wstring const & AccountName;
            std::wstring const & get_AccountName() const { return accountName_; }
            void put_AccountName(std::wstring const & value) { accountName_ = value; }

            __declspec(property(get=get_Namespace, put=put_Namespace)) std::wstring const & Namespace;
            std::wstring const & get_Namespace() const { return namespace_; }
            void put_Namespace(std::wstring const & value) { namespace_ = value; }

            __declspec(property(get=get_MAConfigUrl, put=put_MAConfigUrl)) std::wstring const & MAConfigUrl;
            std::wstring const & get_MAConfigUrl() const { return maConfigUrl_; }
            void put_MAConfigUrl(std::wstring const & value) { maConfigUrl_ = value; }

            __declspec(property(get=get_AutoKeyConfigUrl, put=put_AutoKeyConfigUrl)) std::wstring const & AutoKeyConfigUrl;
            std::wstring const & get_AutoKeyConfigUrl() const { return autoKeyConfigUrl_; }
            void put_AutoKeyConfigUrl(std::wstring const & value) { autoKeyConfigUrl_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(L"accountName", accountName_)
                SERIALIZABLE_PROPERTY(L"namespace", namespace_)
                SERIALIZABLE_PROPERTY(L"maConfigUrl", maConfigUrl_)
                SERIALIZABLE_PROPERTY(L"fluentdConfigUrl", fluentdConfigUrl_)
                SERIALIZABLE_PROPERTY(L"autoKeyConfigUrl", autoKeyConfigUrl_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_05(accountName_, namespace_, maConfigUrl_, fluentdConfigUrl_, autoKeyConfigUrl_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
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
            std::wstring accountName_;
            std::wstring namespace_;
            std::wstring maConfigUrl_;
            std::wstring fluentdConfigUrl_;
            std::wstring autoKeyConfigUrl_;
        };

        DEFINE_DIAGNOSTICS_SINK_ACTIVATOR(AzureInternalMonitoringPipelineSink, DiagnosticsSinkKind::Enum::AzureInternalMonitoringPipeline)
    }
}