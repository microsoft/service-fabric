// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    INITIALIZE_TEMPLATED_SIZE_ESTIMATION(NamePropertyResult)
    INITIALIZE_TEMPLATED_SIZE_ESTIMATION(NamePropertyStoreData)
    
    template<>
    size_t NamePropertyResult::EstimateSerializedSize(size_t uriLength, size_t propertyNameLength, size_t byteCount)
    {
        return
            (uriLength * sizeof(wchar_t) * 2) +
            (propertyNameLength * sizeof(wchar_t)) +
            byteCount +
            SerializationOverheadEstimate;
    }
}
