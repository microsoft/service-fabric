// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Naming
{
    INITIALIZE_SIZE_ESTIMATION( EnumeratePropertiesResult )

    size_t EnumeratePropertiesResult::EstimateSerializedSize(NamePropertyResult const & property)
    {
        return property.EstimateSize() +
            SizeEstimatorHelper::EvaluateDynamicSize(property.Metadata.PropertyName) + // ContinuationToken.LastEnumeratedPropertyName
            SerializationOverheadEstimate;
    }

    size_t EnumeratePropertiesResult::EstimateSerializedSize(size_t uriLength, size_t propertyNameLength, size_t byteCount)
    {
        return 
            NamePropertyResult::EstimateSerializedSize(uriLength, propertyNameLength, byteCount) +
            (propertyNameLength * sizeof(wchar_t)) + // ContinuationToken.LastEnumeratedPropertyName
            SerializationOverheadEstimate;
    }
}
