// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;

    PropertyBatchInfo::PropertyBatchInfo()
        : kind_(PropertyBatchInfoType::Invalid)
        , errorMessage_()
        , operationIndex_(FABRIC_INVALID_OPERATION_INDEX)
        , properties_()
    {
    }

    PropertyBatchInfo::PropertyBatchInfo(NamePropertyOperationBatchResult && batchResult)
        : kind_(PropertyBatchInfoType::Invalid)
        , errorMessage_()
        , operationIndex_(FABRIC_INVALID_OPERATION_INDEX)
        , properties_()
    {
        if (batchResult.Error.IsSuccess())
        {
            kind_ = PropertyBatchInfoType::Successful;

            std::vector<NamePropertyResult> properties = batchResult.TakeProperties();
            for (auto itr = properties.begin(); itr != properties.end(); ++itr)
            {
                std::wstring index(StringUtility::ToWString(itr->Metadata.OperationIndex));
                PropertyInfo propInfo = PropertyInfo::Create(std::move(*itr), true);
                properties_.emplace(std::move(index), std::move(propInfo));
            }

            std::vector<NamePropertyMetadataResult> metadata = batchResult.TakeMetadata();
            for (auto itr = metadata.begin(); itr != metadata.end(); ++itr)
            {
                std::wstring index(StringUtility::ToWString(itr->OperationIndex));
                PropertyInfo propInfo = PropertyInfo(std::move(*itr));
                properties_.emplace(std::move(index), std::move(propInfo));
            }
        }
        else
        {
            kind_ = PropertyBatchInfoType::Failed;
            errorMessage_ = batchResult.Error.ErrorCodeValueToString();
            operationIndex_ = batchResult.FailedOperationIndex;
        }
    }

    PropertyBatchInfo::~PropertyBatchInfo()
    {
    }
}
