// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ContainerApiCall: public Common::IFabricJsonSerializable
    {
        public:
            std::wstring const& HttpVerb() const { return httpVerb_; }
            std::wstring const& UriPath() const { return uriPath_; }
            std::wstring const & ContentType() const { return contentType_; }
            std::wstring const& RequestBody() const { return requestBody_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpVerb, httpVerb_, !httpVerb_.empty())
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UriPath, uriPath_)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpContentType, contentType_, (!contentType_.empty() && !requestBody_.empty()))
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpRequestBody, requestBody_, !requestBody_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring httpVerb_;
            std::wstring uriPath_;
            std::wstring contentType_;
            std::wstring requestBody_;
    };
}
