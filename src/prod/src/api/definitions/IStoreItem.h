// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IStoreItem : public IStoreItemMetadata
    {
    public:
        virtual ~IStoreItem() {};

        virtual std::vector<byte> const & GetValue() const = 0;
        virtual std::vector<byte> TakeValue() = 0;
        virtual bool IsDelete() const = 0;
        virtual Api::IKeyValueStoreItemResultPtr ToKeyValueItem() = 0;
    };
}

