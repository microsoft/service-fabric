// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Store;

TransactionSettings::TransactionSettings()
    : serializationBlockSize_(0)
{
} 

TransactionSettings::TransactionSettings(ULONG serializationBlockSize)
    : serializationBlockSize_(serializationBlockSize)
{
} 

ErrorCode TransactionSettings::FromPublicApi(__in FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS const& publicSettings)
{
    serializationBlockSize_ = publicSettings.SerializationBlockSize;

    return ErrorCodeValue::Success;
}
