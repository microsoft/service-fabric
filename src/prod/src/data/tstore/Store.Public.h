// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#include <ktl.h>
#include <KTpl.h> 

namespace Data
{
    namespace TStore
    {
        using ::_delete;
    }
}

#include "NullableStringStateSerializer.h"
#include "StoreBehavior.h"
#include "StringStateSerializer.h"
#include "StoreInitializationParameters.h"
#include "KBufferSerializer.h"
#include "KBufferComparer.h"
#include "ReadMode.h"
#include "StoreTransactionSettings.h"
#include "IStoreTransaction.h"
#include "IDictionaryChangeHandler.h"
#include "IStore.h"

#include "StoreStateProviderFactory.h"
