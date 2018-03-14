// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class CopyContextData : public Serialization::FabricSerializable
    {
    public:
        CopyContextData() 
            : id_()
            , isEpochValid_(false)
            , lastOperationLSN_(0) 
            , isFileStreamFullCopySupported_(false) // old secondaries (< 4.2, see CopyType.h) are unable to process physical full copies
            , replicaId_(0)
        { 
            epoch_.DataLossNumber = 0;
            epoch_.ConfigurationNumber = 0;
        }

        CopyContextData(
            std::wstring const & id,
            bool isEpochValid,
            ::FABRIC_EPOCH const & epoch, 
            ::FABRIC_SEQUENCE_NUMBER lastOperationLSN,
            ::FABRIC_REPLICA_ID replicaId)
            : id_(id) 
            , isEpochValid_(isEpochValid)
            , epoch_(epoch)
            , lastOperationLSN_(lastOperationLSN)
#if defined(PLATFORM_UNIX)
            , isFileStreamFullCopySupported_(false)
#else
            // New secondaries are running code that supports processing physical
            // full copy. The settings on the primary will determine whether or
            // not a physical full copy will be performed.
            // 
            // This makes it possible to manually change the full copy mode of a
            // partition by updating only the primary replica settings.
            //
            , isFileStreamFullCopySupported_(true)
#endif
            , replicaId_(replicaId)
        {
        }

        __declspec(property(get=get_Id)) std::wstring const & Id;
        __declspec(property(get=get_IsEpochValid)) bool IsEpochValid;
        __declspec(property(get=get_Epoch)) ::FABRIC_EPOCH const & Epoch;
        __declspec(property(get=get_LastOperationLSN)) ::FABRIC_SEQUENCE_NUMBER LastOperationLSN;
        __declspec(property(get=get_IsFileStreamFullCopySupported)) bool IsFileStreamFullCopySupported;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID ReplicaId;

        std::wstring const & get_Id() const { return id_; };
        bool get_IsEpochValid() const { return isEpochValid_; };
        ::FABRIC_EPOCH const & get_Epoch() const { return epoch_; };
        ::FABRIC_SEQUENCE_NUMBER get_LastOperationLSN() const { return lastOperationLSN_; };
        bool get_IsFileStreamFullCopySupported() const { return isFileStreamFullCopySupported_; }
        ::FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

        FABRIC_FIELDS_06(id_, isEpochValid_, epoch_, lastOperationLSN_, isFileStreamFullCopySupported_, replicaId_);

    private:
        std::wstring id_;
        bool isEpochValid_;
        ::FABRIC_EPOCH epoch_;
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN_;
        bool isFileStreamFullCopySupported_;
        ::FABRIC_REPLICA_ID replicaId_;
    };
}

