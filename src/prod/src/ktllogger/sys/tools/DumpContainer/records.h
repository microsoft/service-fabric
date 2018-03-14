// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LogRecords
{
#pragma pack(push)
#pragma pack(1)

    //
    // Record formats and enumerations must be kept consistent with the formats and enums in
    // the transactional replicator sources
    //
    enum
    {
        Invalid =               0,

        // Logical log records
        BeginTransaction =      1,
        Operation =             2,
        EndTransaction =        3,
        Barrier =               4,
        UpdateEpoch =           5,
        Backup =                6,

        // Physical log records
        BeginCheckpoint =       7,
        EndCheckpoint =         8,
        Indexing =              9,
        TruncateHead =          10,
        TruncateTail =          11,
        Information =           12,
        CompleteCheckpoint =    13
    } REPLICATOR_RECORD_TYPE, *PREPLICATOR_RECORD_TYPE;

    CHAR* const ReplicatorRecordTypes[] =
    {
        "Invalid",
        "BeginTransaction",
        "Operation",
        "EndTransaction",
        "Barrier",
        "UpdateEpoch",
        "Backup",
        "BeginCheckpoint",
        "EndCheckpoint",
        "Indexing",
        "TruncateHead",
        "TruncateTail",
        "Information",
        "CompleteCheckpoint"
    };

    enum
    {
        InvalidEvent =          0,
        Recovered =             1,
        CopyFinished =          2,
        ReplicationFinished =   3,
        Closing =               4,
        Closed =                5,
        PrimarySwap =           6,
        RestoredFromBackup =    7
    } INFORMATIONEVENT, *PINFORMATIONEVENT;

    CHAR* const InformationEvent[] = {
        "Invalid",
        "Recovered",
        "CopyFinished",
        "ReplicationFinished",
        "Closing",
        "Closed",
        "PrimarySwap",
        "RestoredFromBackup"
    };

    // LOG RECORD
    typedef struct
    {
        ULONG       RecordLength;
        ULONG       LogicalSize;
        ULONG       RecordType;
        ULONGLONG   LSN;
        ULONG       PhysicalSize;
        ULONGLONG   PSN;
        ULONGLONG   PreviousRecordOffset;

        // Each record type may have additional information.

        // last DWORD of record is Record length

    } REPLICATOR_RECORD_HEADER, *PREPLICATOR_RECORD_HEADER;

#pragma region Logical Log Records
    typedef struct
    {
        ULONG       Size;
    } REPLICATOR_LOGICAL_RECORD, *PREPLICATOR_LOGICAL_RECORD;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER    Header;
        REPLICATOR_LOGICAL_RECORD   Logical;

        ULONG                       Size;
        ULONGLONG                   LastStableLSN;
    } REPLICATOR_BARRIER, *PREPLICATOR_BARRIER;

    typedef struct
    {
        ULONG       LogicalSize;
        ULONGLONG   TransactionId;
        ULONG       PhysicalSize;
        ULONGLONG   ParentTransactionOffset;
    } REPLICATOR_TRANSACTION_RECORD, *PREPLICATOR_TRANSACTION_RECORD;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER        Header;
        REPLICATOR_LOGICAL_RECORD       Logical;
        REPLICATOR_TRANSACTION_RECORD   Transaction;

        ULONG                           Size;
        UCHAR                           IsSingleOperationTransaction;

        // Operation Log Records have three operation datas: metadata, redo and undo.
        // For now just deserializing the metadata operation data count. 
        // This is commonly the most important since multiple layers modify it.
        ULONG                           NumberOfArraySegmentsInMetadataOperationData;
    } REPLICATOR_BEGINTRANSACTION, *PREPLICATOR_BEGINTRANSACTION;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER        Header;
        REPLICATOR_LOGICAL_RECORD       Logical;
        REPLICATOR_TRANSACTION_RECORD   Transaction;

        ULONG                           Size;
        UCHAR                           IsRedoOnly;

        //
        // If IsRedoOnly TRUE then only redo bytes. If FALSE then undo bytes follow
        //
        UCHAR                           Bytes[1];
    } REPLICATOR_OPERATION, *PREPLICATOR_OPERATION;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER        Header;
        REPLICATOR_LOGICAL_RECORD       Logical;
        REPLICATOR_TRANSACTION_RECORD   Transaction;

        ULONG                           Size;
        UCHAR                           IsCommitted;
    } REPLICATOR_ENDTRANSACTION, *PREPLICATOR_ENDTRANSACTION;
#pragma endregion

#pragma region Physical Log Records
    typedef struct
    {
        ULONG       Size;
        ULONGLONG   LinkedPhysicalRecordOffset;
    } REPLICATOR_PHYSICAL_RECORD, *PREPLICATOR_PHYSICAL_RECORD;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER Header;
        REPLICATOR_PHYSICAL_RECORD PhysicalRecord;

        ULONG Size;
        ULONG InformationEvent;
    } REPLICATOR_INFORMATION, *PREPLICATOR_INFORMATION;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER    Header;
        REPLICATOR_LOGICAL_RECORD   Logical;

        ULONG                       Size;
        LONGLONG                    DataLossNumber;
        LONGLONG                    ConfigurationNumber;
        LONGLONG                    ReplicaId;
        UCHAR                       Timestamp[8];
    } REPLICATOR_UPDATEEPOCH, *PREPLICATOR_UPDATEEPOCH;

    typedef struct
    {
        LONGLONG DataLossNumber;
        LONGLONG ConfigurationNumber;

        ULONG Size;
        ULONGLONG LSN;
        ULONGLONG PSN;
        ULONG LogHeadRecordOffset;
    } REPLICATOR_LOGHEAD_RECORD, *PREPLICATOR_LOGHEAD_RECORD;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER Header;
        REPLICATOR_PHYSICAL_RECORD PhysicalRecord;
        REPLICATOR_LOGHEAD_RECORD LogHead;

    } REPLICATOR_TRUNCATEHEAD, *PREPLICATOR_TRUNCATEHEAD;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER Header;
        REPLICATOR_PHYSICAL_RECORD PhysicalRecord;

        ULONG Size;
    } REPLICATOR_TRUNCATETAIL, *PREPLICATOR_TRUNCATETAIL;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER Header;
        REPLICATOR_PHYSICAL_RECORD PhysicalRecord;

        ULONG Size;
        LONG ProgressVectorCount;
        UCHAR ProgressVectors[1];       // Variable sized depends on number progress vectors
        ULONG EarliestPendingTransactionOffset;
        LONGLONG DataLossNumber;
        LONGLONG ConfigurationNumber;
    } REPLICATOR_BEGINCHECKPOINT, *PREPLICATOR_BEGINCHECKPOINT;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER Header;
        REPLICATOR_LOGHEAD_RECORD LogHead;
        REPLICATOR_PHYSICAL_RECORD PhysicalRecord;

        ULONG Size;
        ULONGLONG LastStableLSN;
        ULONG LastCompletedBeginCheckpointOffset;
    } REPLICATOR_ENDCHECKPOINT, *PREPLICATOR_ENDCHECKPOINT;

    typedef struct
    {
        REPLICATOR_RECORD_HEADER    Header;
        REPLICATOR_PHYSICAL_RECORD  PhysicalRecord;

        ULONG       Size;
        LONGLONG    DataLossNumber;
        LONGLONG    ConfigurationNumber;
    } REPLICATOR_INDEXING, *PREPLICATOR_INDEXING;
#pragma endregion

#pragma pack(pop)
}
