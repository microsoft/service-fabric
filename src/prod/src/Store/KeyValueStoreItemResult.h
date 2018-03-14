// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreItemResult : 
        public Common::ComponentRoot,
        public Api::IKeyValueStoreItemResult
    {
        DENY_COPY(KeyValueStoreItemResult);

    public:
        KeyValueStoreItemResult(
            std::wstring const & key,
            std::vector<BYTE> && value,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            FILETIME lastModifiedUtc,
            FILETIME lastModifiedOnPrimaryUtc);
        virtual ~KeyValueStoreItemResult();

        //
        // IKeyValueStoreItemResult methods
        //
        FABRIC_KEY_VALUE_STORE_ITEM const * get_Item();

    private:
        std::wstring key_;
        std::vector<BYTE> value_;
        FABRIC_KEY_VALUE_STORE_ITEM_METADATA itemMetadata_;
        FABRIC_KEY_VALUE_STORE_ITEM_METADATA_EX1 itemMetadataEx1_;
        FABRIC_KEY_VALUE_STORE_ITEM item_;
    };
}
