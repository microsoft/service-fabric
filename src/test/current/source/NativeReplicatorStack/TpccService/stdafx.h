// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <atomic>
#include "../StatefulServiceBase/StatefulServiceBase.Public.h"
#include "../TestableService/TestableService.Public.h"
#include "../../prod/src/data/utilities/Data.Utilities.Public.h"
#include "../../prod/src/data/txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../../prod/src/data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"
//#include "../../prod/src/data/tstore/Store.Public.h"
#include "../../prod/src/data/tstore/stdafx.h"

namespace TpccService
{
    using ::_delete;
}

#define PROPERTY(TYPE, var, name)\
    _declspec(property(get = get_##name, put = set_##name)) TYPE name;\
    TYPE& get_##name()\
    {\
        return var;\
    }\
    TYPE get_##name() const\
    {\
        return var;\
    }\
    void set_##name(const TYPE& val)\
    {\
        var = val;\
    }

#define KSTRING_PROPERTY(var, name)\
    PROPERTY(KString::SPtr, var, name)\
    void set_##name(const std::wstring& val)\
    {\
        KString::Create(var, this->GetThisAllocator(), val.c_str());\
    }


#include "VariableText.h"
#include "FixedText.h"
#include "CreateStoreUtil.h"
#include "IntSerializer.h"
#include "Long64Serializer.h"
#include "WarehouseValue.h"
#include "WarehouseValueByteSerializer.h"
#include "WarehouseStoreUtil.h"
#include "DistrictValue.h"
#include "DistrictValueByteSerializer.h"
#include "DistrictKey.h"
#include "DistrictKeyComparer.h"
#include "DistrictKeySerializer.h"
#include "DistrictStoreUtil.h"
#include "CustomerKey.h"
#include "CustomerKeyComparer.h"
#include "CustomerKeySerializer.h"
#include "CustomerValue.h"
#include "CustomerValueSerializer.h"
#include "CustomerStoreUtil.h"
#include "NewOrderKey.h"
#include "NewOrderKeyComparer.h"
#include "NewOrderKeySerializer.h"
#include "NewOrderValue.h"
#include "NewOrderValueSerializer.h"
#include "NewOrderStoreUtil.h"
#include "OrderKey.h"
#include "OrderKeyComparer.h"
#include "OrderKeySerializer.h"
#include "OrderValue.h"
#include "OrderValueSerializer.h"
#include "OrderStoreUtil.h"
#include "OrderLineKey.h"
#include "OrderLineKeyComparer.h"
#include "OrderLineKeySerializer.h"
#include "OrderLineValue.h"
#include "OrderLineValueSerializer.h"
#include "OrderLineStoreUtil.h"
#include "ItemValue.h"
#include "ItemValueSerializer.h"
#include "ItemStoreUtil.h"
#include "StockKey.h"
#include "StockKeyComparer.h"
#include "StockKeySerializer.h"
#include "StockValue.h"
#include "StockValueSerializer.h"
#include "StockStoreUtil.h"
#include "GuidComparer.h"
#include "GuidSerializer.h"
#include "HistoryValue.h"
#include "HistoryValueSerializer.h"
#include "HistoryStoreUtil.h"

#include "TestComStateProvider2Factory.h"
#include "ClientOperationEnum.h"
#include "WorkStatusEnum.h"
#include "HttpPostRequest.h"
#include "ServiceProgress.h"
#include "ServiceMetrics.h"
#include "HttpPostResponse.h"
#include "Service.h"

#undef KSTRING_PROPERTY
#undef PROPERTY
