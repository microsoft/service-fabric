// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerApiExecutionArgs : public Serialization::FabricSerializable
    {
    public:
        ContainerApiExecutionArgs();

        ContainerApiExecutionArgs(
            std::wstring const & containerName,
            std::wstring const & httpVerb,
            std::wstring const & uriPath,
            std::wstring const & contentType,
            std::wstring const & requestBody);

        ContainerApiExecutionArgs(ContainerApiExecutionArgs const & other) = default;
        ContainerApiExecutionArgs(ContainerApiExecutionArgs && other) = default;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_API_EXECUTION_ARGS & apiExecutionArgs) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(ContainerName, HttpVerb, UriPath, ContentType, RequestBody);

    public:
        std::wstring ContainerName;
        std::wstring HttpVerb;
        std::wstring UriPath;
        std::wstring ContentType;
        std::wstring RequestBody;
    };
}
