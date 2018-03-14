// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data;
using namespace ktl;
using namespace ServiceModel;
using namespace std;
using namespace Store;

TSEnumerationBase::TSEnumerationBase(
    ::Store::PartitionedReplicaId const & partitionedReplicaId,
    wstring const & type, 
    wstring const & keyPrefix,
    bool strictPrefix)
    : PartitionedReplicaTraceComponent(partitionedReplicaId)
    , targetType_(type)
    , targetKeyPrefix_(keyPrefix)
    , strictPrefix_(strictPrefix)
    , isInnerEnumInitialized_(false)
    , currentType_()
    , currentKey_()
{
}

TSEnumerationBase::~TSEnumerationBase()
{
}

ErrorCode TSEnumerationBase::OnMoveNextBase()
{
    TRY_CATCH_ERROR( this->InnerMoveNext() );

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumerationBase::InnerMoveNext()
{
    // TStore will initialize the underlying enumerator by seeking
    // to the first relevant key when an enumeration API is called
    // by the application. In this case, no additional looping will
    // occur.
    //
    // For copy notifications, the underlying enumeration is simply
    // pointing before the first entry for both TStore and TSChangeHandler,
    // so we have to initialize by seeking to the first relevant key here.
    //
    // TODO: This needs to be optimized for the copy notification case.
    // Data::Utilities::IAsyncEnumerator<T> needs to expose an
    // API to facilitate efficient seeking to the first relevant key.
    //
    while (this->OnInnerMoveNext())
    {
        auto kString = this->OnGetCurrentKey();

        this->SplitKey(kString, currentType_, currentKey_);

        WriteNoise(
            this->GetTraceComponent(), 
            "{0} MoveNext: raw='{1}' type='{2}' key='{3}'",
            this->GetTraceId(),
            wstring(static_cast<wchar_t *>(*kString), kString->Length()),
            currentType_,
            currentKey_);

        if (!isInnerEnumInitialized_)
        {
            if (currentType_ < targetType_)
            {
                continue;
            }
            else if (currentType_ > targetType_)
            {
                break;
            }

            if (currentKey_ < targetKeyPrefix_)
            {
                continue;
            }

            isInnerEnumInitialized_ = true;
        }
        else if (currentType_ != targetType_)
        {
            break;
        }

        if (!strictPrefix_ || StringUtility::StartsWith(currentKey_, targetKeyPrefix_))
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            break;
        }
    }

    WriteNoise(
        this->GetTraceComponent(), 
        "{0} enumeration completed",
        this->GetTraceId());
            
    return ErrorCodeValue::EnumerationCompleted;
}
