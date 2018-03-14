// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStoreItemMetadata
    {
    public:
        virtual ~IStoreItemMetadata() {};

        virtual _int64 GetOperationLSN() const = 0;
        virtual std::wstring const & GetType() const = 0;
        virtual std::wstring const & GetKey() const = 0;
        virtual Common::DateTime GetLastModifiedUtc() const = 0;
        virtual Common::DateTime GetLastModifiedOnPrimaryUtc() const = 0;
        virtual size_t GetValueSize() const = 0;
        virtual std::wstring TakeType() = 0;
        virtual std::wstring TakeKey() = 0;
        virtual Api::IKeyValueStoreItemMetadataResultPtr ToKeyValueItemMetadata() = 0;
    };
}

