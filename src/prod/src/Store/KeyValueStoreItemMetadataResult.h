// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreItemMetadataResult : 
        public Common::ComponentRoot,
        public Api::IKeyValueStoreItemMetadataResult
    {
        DENY_COPY(KeyValueStoreItemMetadataResult);

    public:
        KeyValueStoreItemMetadataResult(
            std::wstring const & key,
            LONG valueSizeInBytes,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            FILETIME lastModifiedUtc,
            FILETIME lastModifiedOnPrimaryUtc);
        virtual ~KeyValueStoreItemMetadataResult();

        //
        // IKeyValueStoreItemMetadataResult methods
        //
        FABRIC_KEY_VALUE_STORE_ITEM_METADATA  const * get_Metadata();

    private:
        std::wstring key_;
        FABRIC_KEY_VALUE_STORE_ITEM_METADATA itemMetadata_;
        FABRIC_KEY_VALUE_STORE_ITEM_METADATA_EX1 itemMetadataEx1_;
    };
}
