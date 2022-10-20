// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class HttpConfig;
        using HttpConfigSPtr = std::shared_ptr<HttpConfig>;

        class HttpConfig
            : public ModelType
        {
        public:
            __declspec(property(get = get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get = get_ListenPort)) uint const & ListenPort;
            uint const & get_ListenPort() const { return listenPort_; }

            __declspec(property(get = get_HostNames)) std::vector<HttpHostNameConfig> const & HostNames;
            std::vector<HttpHostNameConfig> const & get_HostNames() const { return hostNames_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"name", name_)
                SERIALIZABLE_PROPERTY(L"port", listenPort_)
                SERIALIZABLE_PROPERTY_IF(L"hosts", hostNames_, (hostNames_.size() != 0))
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(name_, listenPort_, hostNames_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(listenPort_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(hostNames_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;

            std::wstring name_;
            uint listenPort_;
            std::vector<HttpHostNameConfig> hostNames_;
        };
    }
}


DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::HttpConfig);
