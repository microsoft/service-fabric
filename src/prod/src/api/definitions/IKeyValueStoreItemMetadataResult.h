// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreItemMetadataResult
    {
    public:
        virtual ~IKeyValueStoreItemMetadataResult() {};

        virtual const FABRIC_KEY_VALUE_STORE_ITEM_METADATA * get_Metadata() = 0;
    };
}
