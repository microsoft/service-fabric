// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class GatewayResourceDescription;
        using GatewayResourceDescriptionSPtr = std::shared_ptr<GatewayResourceDescription>;

        class GatewayProperties
            : public ModelType
        {
        public:
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"description", description_)
                SERIALIZABLE_PROPERTY(L"sourceNetwork", sourceNetwork_)
                SERIALIZABLE_PROPERTY(L"destinationNetwork", destinationNetwork_)
                SERIALIZABLE_PROPERTY_ENUM(Constants::status, status_)
                SERIALIZABLE_PROPERTY(L"statusDetails", statusDetails_)
                SERIALIZABLE_PROPERTY_IF(L"tcp", tcp_, (tcp_.size() != 0))
                SERIALIZABLE_PROPERTY_IF(L"http", http_, (http_.size() != 0))
                SERIALIZABLE_PROPERTY(L"ipAddress", ipAddress_)
                END_JSON_SERIALIZABLE_PROPERTIES()

                FABRIC_FIELDS_08(description_, sourceNetwork_, destinationNetwork_, status_, statusDetails_, tcp_, http_, ipAddress_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(sourceNetwork_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(destinationNetwork_)
                DYNAMIC_ENUM_ESTIMATION_MEMBER(status_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(statusDetails_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(tcp_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(http_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(ipAddress_)
                END_DYNAMIC_SIZE_ESTIMATION()

                Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const
            {
                return;
            }

            std::wstring description_;
            NetworkRef sourceNetwork_;
            NetworkRef destinationNetwork_;
            GatewayResourceStatus::Enum status_;
            std::wstring statusDetails_;
            std::vector<TcpConfig> tcp_;
            std::vector<HttpConfig> http_;
            std::wstring ipAddress_;
        };

        class GatewayResourceDescription
            : public ResourceDescription
        {
        public:
            GatewayResourceDescription()
                : ResourceDescription(*ServiceModel::Constants::ModelV2ReservedDnsNameCharacters)
                , Properties()
            {
            }

            __declspec(property(get = get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return Properties.description_; }

            __declspec(property(get = get_SourceNetwork)) NetworkRef const & SourceNetwork;
            NetworkRef const & get_SourceNetwork() const { return Properties.sourceNetwork_; }

            __declspec(property(get = get_DestinationNetwork)) NetworkRef const & DestinationNetwork;
            NetworkRef const & get_DestinationNetwork() const { return Properties.destinationNetwork_; }

            __declspec(property(get = get_Status)) GatewayResourceStatus::Enum const & Status;
            GatewayResourceStatus::Enum const & get_Status() const { return Properties.status_; }

            __declspec(property(get = get_StatusDetails)) std::wstring const & StatusDetails;
            std::wstring const & get_StatusDetails() const { return Properties.statusDetails_; }

            __declspec(property(get = get_Tcp, put = put_Tcp)) std::vector<TcpConfig> const & Tcp;
            std::vector<TcpConfig> const & get_Tcp() const { return Properties.tcp_; }
            void put_Tcp(std::vector<TcpConfig> const & value) { Properties.tcp_ = value; }

            __declspec(property(get = get_Http, put = put_Http)) std::vector<HttpConfig> const & Http;
            std::vector<HttpConfig> const & get_Http() const { return Properties.http_; }
            void put_Http(std::vector<HttpConfig> const & value) { Properties.http_ = value; }

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(Constants::properties, Properties)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Properties)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_01(Properties)

            std::wstring ToString() const;
            static Common::ErrorCode FromString( std::wstring const & str, __out GatewayResourceDescription & description);

        public:
            GatewayProperties Properties;
        };
        QUERY_JSON_LIST(GatewayResourceDescriptionList, GatewayResourceDescription)
    }
}
