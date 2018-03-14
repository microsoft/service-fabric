// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerApiExecutionResponse : public Serialization::FabricSerializable
    {
    public:
        ContainerApiExecutionResponse();

        ContainerApiExecutionResponse(
            USHORT statusCode,
            std::wstring const & contentType,
            std::wstring const & contentEncoding,
            std::vector<BYTE> && responseBody);

        ContainerApiExecutionResponse(ContainerApiExecutionResponse const & other) = default;
        ContainerApiExecutionResponse(ContainerApiExecutionResponse && other) = default;

        ContainerApiExecutionResponse & operator = (ContainerApiExecutionResponse const & other) = default;
        ContainerApiExecutionResponse & operator = (ContainerApiExecutionResponse && other) = default;

        Common::ErrorCode FromPublicApi(FABRIC_CONTAINER_API_EXECUTION_RESPONSE const & apiExecutionResponse);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(StatusCode, ContentType, ContentEncoding, ResponseBody);

    public:
        USHORT StatusCode;
        std::wstring ContentType;
        std::wstring ContentEncoding;
        std::vector<BYTE> ResponseBody;
    };
}
