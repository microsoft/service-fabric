// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define ESESTORE_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    trace_name(                                                           \
        Common::TraceTaskCodes::EseStore,                                 \
        trace_id,                                                         \
        #trace_name,                                                      \
        Common::LogLevel::##trace_level,                                  \
        __VA_ARGS__)                                                      \

namespace Store
{
    class EseStoreEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(StoreInitialized, StoreInstanceId, std::wstring, std::wstring, bool);
        DECLARE_STRUCTURED_TRACE(StoreDestructing, StoreInstanceId);
        DECLARE_STRUCTURED_TRACE(CreateTransaction, StoreInstanceId, TransactionInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(Insert, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, uint64, int64, int64);
        DECLARE_STRUCTURED_TRACE(Update, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, int64, std::wstring, uint64, int64, int64);
        DECLARE_STRUCTURED_TRACE(UpdateOperationLSN, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(SequenceCheckMismatch, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, int64, int64);
        DECLARE_STRUCTURED_TRACE(OperationLSNTooSmall, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, int64, int64);
        DECLARE_STRUCTURED_TRACE(Delete, StoreInstanceId, TransactionInstanceId, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(TransactionError, StoreInstanceId, TransactionInstanceId, int);
        DECLARE_STRUCTURED_TRACE(Commit, StoreInstanceId, TransactionInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(BeginCommit, StoreInstanceId, TransactionInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(EndCommit, StoreInstanceId, TransactionInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(Rollback, StoreInstanceId, TransactionInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(CreateEnumeration, StoreInstanceId, TransactionInstanceId, EnumerationInstanceId, std::wstring, std::wstring, bool);
        DECLARE_STRUCTURED_TRACE(CreateChangeEnumeration, StoreInstanceId, TransactionInstanceId, EnumerationInstanceId, int64);
        DECLARE_STRUCTURED_TRACE(EnumerationMoveNext, StoreInstanceId, TransactionInstanceId, EnumerationInstanceId, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(EnumerationDestructing, StoreInstanceId, TransactionInstanceId, EnumerationInstanceId, int, bool);

        EseStoreEventSource() :
            ESESTORE_STRUCTURED_TRACE(StoreInitialized,        4,  Noise, "Store[{0}]: Initialized(directory='{1}' filename='{2}' createdNew={3})",                    "storeId", "directory", "filename", "createdNew"),
            ESESTORE_STRUCTURED_TRACE(StoreDestructing,        5,  Noise, "Store[{0}]: Destructing()",                                                                 "storeId"),
            ESESTORE_STRUCTURED_TRACE(CreateTransaction,       6,  Noise, "Store[{0}] Tx[{1},{2}]: CreateTransaction()",                                                  "storeId", "transaction", "sessionId"),
            ESESTORE_STRUCTURED_TRACE(Insert,                  7,  Noise, "Store[{0}] Tx[{1}]: Insert(type='{2}' key='{3}' size={4} seq={5} lsn={6})",                        "storeId", "transactionInstance", "ty", "key", "size", "seq", "lsn"),
            ESESTORE_STRUCTURED_TRACE(Update,                  8,  Noise, "Store[{0}] Tx[{1}]: Update(type='{2}' key='{3}' check={4} newKey='{5}' size={6} seq={7} lsn={8})", "storeId", "transactionInstance", "ty", "key", "check", "newKey", "size", "seq", "lsn"),
            ESESTORE_STRUCTURED_TRACE(UpdateOperationLSN,      9,  Noise, "Store[{0}] Tx[{1}]: UpdateOperationLSN(type='{2}' key='{3}' lsn={4})",         "storeId", "transactionInstance", "ty", "key", "lsn"),
            ESESTORE_STRUCTURED_TRACE(SequenceCheckMismatch,   10, Info, "Store[{0}] Tx[{1}]: SequenceCheckMismatch(type='{2}' key='{3}' check={4} seq={5})",        "storeId", "transactionInstance", "ty", "key", "check", "seq"),
            ESESTORE_STRUCTURED_TRACE(OperationLSNTooSmall,    11, Noise, "Store[{0}] Tx[{1}]: LSNCheckMismatch(type='{2}' key='{3}' oldlsn={4} lsn={5})",       "storeId", "transactionInstance", "ty", "key", "oldlsn", "lsn"),
            ESESTORE_STRUCTURED_TRACE(Delete,                  12, Noise, "Store[{0}] Tx[{1}]: Delete(type='{2}' key='{3}' check={4})",                               "storeId", "transactionInstance", "ty", "key", "check"),
            ESESTORE_STRUCTURED_TRACE(TransactionError,        13, Noise, "Store[{0}] Tx[{1}]: TransactionError(hr='{2:x}')",                                         "storeId", "transactionInstance", "hr"),
            ESESTORE_STRUCTURED_TRACE(Commit,                  14, Noise, "Store[{0}] Tx[{1},{2}]: Commit()",                                                             "storeId", "transactionInstance", "sessionId"),
            ESESTORE_STRUCTURED_TRACE(BeginCommit,             15, Noise, "Store[{0}] Tx[{1},{2}]: BeginCommit()",                                               "storeId", "transactionInstance", "sessionId"),
            ESESTORE_STRUCTURED_TRACE(EndCommit,               16, Noise, "Store[{0}] Tx[{1},{2}]: EndCommit()",                                                          "storeId", "transactionInstance", "sessionId"),
            ESESTORE_STRUCTURED_TRACE(Rollback,                17, Noise, "Store[{0}] Tx[{1},{2}]: Rollback()",                                                           "storeId", "transactionInstance", "sessionId"),
            ESESTORE_STRUCTURED_TRACE(CreateEnumeration,       18, Noise, "Store[{0}] Tx[{1}] Enum[{2}]: CreateEnumeration(type='{3}' keyStart='{4}' strictType={5})",               "storeId", "transaction", "enumeration", "ty", "keyStart", "strictType"),
            ESESTORE_STRUCTURED_TRACE(CreateChangeEnumeration, 19, Noise, "Store[{0}] Tx[{1}] Enum[{2}]: CreateChangeEnumeration(seq={3})",                           "storeId", "transaction", "enumeration", "seq"),
            ESESTORE_STRUCTURED_TRACE(EnumerationMoveNext,     20, Noise, "Store[{0}] Tx[{1}] Enum[{2}]: MoveNext(type='{3}' key='{4}' seq={5})",                     "storeId", "transaction", "enumeration", "ty", "key", "seq"),
            ESESTORE_STRUCTURED_TRACE(EnumerationDestructing,  21, Noise, "Store[{0}] Tx[{1}] Enum[{2}]: Destructing(count={3} completed={4})",                       "storeId", "transaction", "enumeration", "count", "completed")
        {
        }

        static Common::Global<EseStoreEventSource> Trace;
    };
}
