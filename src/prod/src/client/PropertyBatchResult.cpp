// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;

PropertyBatchResult::PropertyBatchResult(NamePropertyOperationBatchResult &&result)
    : properties_()
    , propertiesNotFound_(move(result.TakePropertiesNotFound()))
    , failedOperationIndex_(result.FailedOperationIndex)
    , error_(result.Error)
{
    // Add entries for all existing properties
    for (auto iter = result.Properties.begin(); iter != result.Properties.end(); ++iter)
    {
        NamePropertyResult & propertyResult = const_cast<NamePropertyResult &>(*iter);

        ULONG index = propertyResult.Metadata.OperationIndex;

        FabricClientImpl::WriteNoise(
            Constants::FabricClientSource,
            "Adding property {0} at index {1}",
            propertyResult.Metadata.PropertyName,
            index);

        properties_[index] = move(propertyResult);
    }

    // Add entries for all existing metadata
    for (auto iter = result.Metadata.begin(); iter != result.Metadata.end(); ++iter)
    {
        NamePropertyMetadataResult & propertyMetadata = const_cast<NamePropertyMetadataResult &>(*iter);

        ULONG index = propertyMetadata.OperationIndex;
        ByteBuffer bytes;

        FabricClientImpl::WriteNoise(
            Constants::FabricClientSource,
            "Adding property metadata {0} at index {1}",
            propertyMetadata.PropertyName,
            index);

        properties_[index] = move(NamePropertyResult(move(propertyMetadata), move(bytes)));
    }    
}

ULONG PropertyBatchResult::GetFailedOperationIndexInRequest()
{
    return failedOperationIndex_;
}

ErrorCode PropertyBatchResult::GetError()
{
    return error_;
}

ErrorCode PropertyBatchResult::GetProperty(__in ULONG operationIndexInRequest, __out NamePropertyResult &result)
{
    auto it = properties_.find(operationIndexInRequest);
    if (it != properties_.end())
    {
        // The index represents a GetProperty operation
        result = it->second;
        return ErrorCode::Success();
    }

    // Check whether the index represents a Get operation
    // for a property that doesn't exist
    for (auto index : propertiesNotFound_)
    {
        if (index == operationIndexInRequest)
        {
            return ErrorCodeValue::PropertyNotFound;
        }
    }

    return ErrorCodeValue::InvalidArgument;
}
