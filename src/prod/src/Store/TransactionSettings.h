// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class TransactionSettings
    {
    public:
        TransactionSettings();
        TransactionSettings(ULONG serializationBlockSize);

        __declspec(property(get=get_SerializationBlockSize)) ULONG SerializationBlockSize;
        ULONG get_SerializationBlockSize() const { return serializationBlockSize_; }

        Common::ErrorCode FromPublicApi(__in FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS const&);

    private:
        ULONG serializationBlockSize_;
    };
}
