// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability
{
    namespace ReplicationComponent
    {
        using Common::ComUtility;

        ComUserDataOperation::ComUserDataOperation(
            Common::ComPointer<IFabricOperationData> && comOperationDataPointer,
            FABRIC_OPERATION_METADATA const & metadata)
            :   ComOperation(metadata, Constants::InvalidEpoch, metadata.SequenceNumber), 
                operation_(std::move(comOperationDataPointer))
        {
        }

        ComUserDataOperation::ComUserDataOperation(
            Common::ComPointer<IFabricOperationData> && comOperationDataPointer,
            FABRIC_OPERATION_METADATA const & metadata, 
            FABRIC_EPOCH const & epoch)
            :   ComOperation(metadata, epoch, metadata.SequenceNumber), 
                operation_(std::move(comOperationDataPointer))
        {
        }

        ComUserDataOperation::~ComUserDataOperation()
        {
        }

        HRESULT ComUserDataOperation::Acknowledge()
        {
            return ComUtility::OnPublicApiReturn(S_OK);
        }

        bool ComUserDataOperation::IsEmpty() const
        {
            return operation_.GetRawPointer() == nullptr;
        }

        HRESULT ComUserDataOperation::GetData(
            /*[out]*/ ULONG * size, 
            /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** value)
        {
            if (operation_.GetRawPointer() == nullptr)
            {
                // The operation is empty
                // TODO: 134637: consider whether NULL operations 
                // should be treated differently than empty operations
                *size = 0;
                *value = nullptr;
                return ComUtility::OnPublicApiReturn(S_OK);
            }

            return ComUtility::OnPublicApiReturn(operation_->GetData(size, value));
        }
    }// end namespace ReplicationComponent
} // end namespace Reliability
