// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IKeyValueStoreItemMetadataEnumerator
    {
    public:
        virtual ~IKeyValueStoreItemMetadataEnumerator() {};

        virtual Common::ErrorCode MoveNext() = 0;
        virtual Common::ErrorCode TryMoveNext(bool & success) = 0;
        virtual IKeyValueStoreItemMetadataResultPtr get_Current() = 0;
    };
}
