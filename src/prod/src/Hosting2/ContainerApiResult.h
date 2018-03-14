// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Hosting2/Constants.h"

namespace Hosting2
{
    class ContainerApiResult : public Common::IFabricJsonSerializable
    {
    public:
        ContainerApiResult(
            unsigned status,
            std::wstring && contentType,
            std::wstring && contentEncoding,
            std::wstring && body);

        ContainerApiResult() = default;
        ContainerApiResult(ContainerApiResult const &) = default;
        ContainerApiResult(ContainerApiResult &&) = default;

        ContainerApiResult & operator = (ContainerApiResult const &) = default;
        ContainerApiResult & operator = (ContainerApiResult &&) = default;

        unsigned Status() const { return status_; }
        std::wstring const & ContentType() const { return contentType_; }
        std::wstring const & ContentEncoding() const { return contentEncoding_; }
        std::wstring const & Body() const { return body_; }

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::HttpStatus, status_)
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpContentType, contentType_, (!contentType_.empty() && !body_.empty()))
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpContentEncoding, contentEncoding_, (!contentEncoding_.empty() && !body_.empty()))
            SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::HttpResponseBody, body_, !body_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        unsigned status_;
        std::wstring contentType_;
        std::wstring contentEncoding_;
        std::wstring body_;
    };

    class ContainerApiResponse : public Common::IFabricJsonSerializable
    {
    public:
        ContainerApiResponse(
            unsigned status,
            std::wstring && contentType,
            std::wstring && contentEncoding,
            std::wstring && body);

        ContainerApiResponse() = default;

        ContainerApiResult const & Result() { return result_; }

        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ContainerApiResult, result_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ContainerApiResult result_;
    };
}
