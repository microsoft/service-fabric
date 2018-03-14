// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStoreEnumerator
    {
    public:
        virtual ~IStoreEnumerator() {};

        virtual Common::ErrorCode Enumerate(
            std::wstring const & type,
            std::wstring const & keyPrefix,
            bool strictPrefix,
            __out IStoreItemEnumeratorPtr & itemEnumerator) = 0;

        virtual Common::ErrorCode EnumerateMetadata(
            std::wstring const & type,
            std::wstring const & keyPrefix,
            bool strictPrefix,
            __out IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) = 0;

        virtual Common::ErrorCode EnumerateKeyValueStore(
            std::wstring const & keyPrefix,
            bool strictPrefix,
            __out IStoreItemEnumeratorPtr & itemEnumerator) = 0;

        virtual Common::ErrorCode EnumerateKeyValueStoreMetadata(
            std::wstring const & keyPrefix,
            bool strictPrefix,
            __out IStoreItemMetadataEnumeratorPtr & itemMetadataEnumerator) = 0;
    };
}
