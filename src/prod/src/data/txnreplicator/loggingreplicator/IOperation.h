// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Describes the data that is obtained from the state replicator.
        interface IOperation
        {
            K_SHARED_INTERFACE(IOperation)

        public:
            virtual void Acknowledge() = 0;

            __declspec(property(get = get_OperationType)) FABRIC_OPERATION_TYPE OperationType;
            virtual FABRIC_OPERATION_TYPE get_OperationType() const = 0;

            __declspec(property(get = get_SequenceNumber)) LONG64 SequenceNumber;
            virtual LONG64 get_SequenceNumber() const = 0;

            __declspec(property(get = get_AtomicGroupId)) LONG64 AtomicGroupId;
            virtual LONG64 get_AtomicGroupId() const = 0;

            __declspec(property(get = get_Data)) Utilities::OperationData::CSPtr Data;
            virtual Utilities::OperationData::CSPtr get_Data() const = 0;
        };
    }
}
