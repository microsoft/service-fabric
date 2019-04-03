// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Store/test/Common.Test.h"

using namespace std;
using namespace Common;

namespace Store
{
    GlobalWString TestStoreInstanceName = make_global<std::wstring>(L"TestEseLocalStore");

#define VERIFY(condition, ...) VERIFY_IS_TRUE(condition, wformatString(__VA_ARGS__).c_str());

    struct IndexRange;

    class TestEseLocalStore
    {
    protected:
        TestEseLocalStore() { BOOST_REQUIRE(TestcaseSetup()); }
        TEST_METHOD_SETUP(TestcaseSetup)

        std::shared_ptr<EseLocalStore> CreateTestLocalStore(TemporaryCurrentDirectory const &, LONG flags = EseLocalStore::LOCAL_STORE_FLAG_NONE, bool enableIncrementalBackup = false);
        void RestoreHelper(TemporaryCurrentDirectory const & baseDir, IndexRange const & range);
        void UpgradeToIncrementalBackupHelper(TemporaryCurrentDirectory const & baseDir);
        void TruncateHelper(
            TemporaryCurrentDirectory const & baseDir,
            bool enableIncrementalBackup,
            IndexRange const & creationRange,
            IndexRange const & verificationRange);
    };

    class ITableGenerator
    {
    public:
        virtual void GetType(/*out*/wstring & type) const = 0;
        virtual void GetKey(int index, /*out*/wstring & key) const = 0;
        virtual void GetValue(int index, /*out*/vector<unsigned _int8> & value) const = 0;
    };

    struct StoreItem
    {
        wstring Type;
        wstring Key;
        vector<unsigned _int8> Value;
    };

    struct IndexRange
    {
        IndexRange(int limit) : Start(0), Step(1), Limit(limit) { }
        IndexRange(int start, int limit) : Start(start), Step(1), Limit(limit) { }
        IndexRange(int start, int step, int limit) : Start(start), Step(step), Limit(limit) { }
        int Start;
        int Step;
        int Limit;
    };

    enum CheckSequenceType { NoCheck, MatchCurrentSN, LowerThanCurrentSN, HigherThanCurrentSN };

    class TableValidator
    {
        DENY_COPY(TableValidator);
    public:
        TableValidator(ILocalStore & store) : store_(store) { }

        void BulkInsert(
            ITableGenerator const & generator,
            IndexRange const & range) const
        {
            CODING_ERROR_ASSERT(range.Start >= 0);
            ILocalStore::TransactionSPtr transaction;
            CODING_ERROR_ASSERT(store_.CreateTransaction(transaction).IsSuccess());
            StoreItem item;

            generator.GetType(/*out*/item.Type);
            for (int i = range.Start; i<range.Limit; i += range.Step)
            {
                generator.GetKey(i, /*out*/item.Key);
                generator.GetValue(i, /*out*/item.Value);

                this->DumpItem(L"Insert ", item);

                CODING_ERROR_ASSERT(
                    store_.Insert(
                        transaction,
                        item.Type,
                        item.Key,
                        item.Value.data(),
                        item.Value.size(),
                        TableValidator::OperationNumberAtInsert).IsSuccess());
            }

            CODING_ERROR_ASSERT(transaction->Commit().IsSuccess());
        }


        void BulkUpdate(
            ITableGenerator const & prevGen,
            ITableGenerator const & nextGen,
            IndexRange const & range) const
        {
            this->BulkUpdate(prevGen, nextGen, range, CheckSequenceType::NoCheck, true);
        }

        void BulkUpdate(
            ITableGenerator const & prevGen,
            ITableGenerator const & nextGen,
            IndexRange const & range,
            CheckSequenceType checkSequenceType,
            bool expectSuccess) const
        {
            ILocalStore::TransactionSPtr transaction;

            StoreItem prev;
            StoreItem next;
            prevGen.GetType(/*out*/prev.Type);
            nextGen.GetType(/*out*/next.Type);
            CODING_ERROR_ASSERT(prev.Type == next.Type);

            for (int i = range.Start; i<range.Limit; i += range.Step)
            {
                CODING_ERROR_ASSERT(store_.CreateTransaction(transaction).IsSuccess());

                _int64 checkSequenceNumber = ILocalStore::SequenceNumberIgnore;

                prevGen.GetKey(i, /*out*/prev.Key);
                nextGen.GetKey(i, /*out*/next.Key);
                if (next.Key == prev.Key) { next.Key.clear(); }
                nextGen.GetValue(i, /*out*/next.Value);

                if (CheckSequenceType::NoCheck != checkSequenceType)
                {
                    ILocalStore::EnumerationSPtr enumSPtr;
                    ErrorCode error = store_.CreateEnumerationByTypeAndKey(transaction, prev.Type, prev.Key, enumSPtr);
                    CODING_ERROR_ASSERT(error.IsSuccess());

                    while ((error = enumSPtr->MoveNext()).IsSuccess())
                    {
                        std::wstring currentType;
                        if (!(error = enumSPtr->CurrentType(currentType)).IsSuccess())
                        {
                            break;
                        }

                        if (0 == prev.Type.compare(currentType))
                        {
                            std::wstring currentKey;
                            if (!(error = enumSPtr->CurrentKey(currentKey)).IsSuccess())
                            {
                                break;
                            }

                            if (0 == prev.Key.compare(currentKey))
                            {
                                if (!(error = enumSPtr->CurrentOperationLSN(checkSequenceNumber)).IsSuccess())
                                {
                                    break;
                                }
                                break;
                            }
                        }
                    }

                    CODING_ERROR_ASSERT(error.IsSuccess());

                    if (CheckSequenceType::LowerThanCurrentSN == checkSequenceType)
                    {
                        checkSequenceNumber--;
                    }
                    else if (CheckSequenceType::HigherThanCurrentSN == checkSequenceType)
                    {
                        checkSequenceNumber++;
                    }
                }

                wstring s;
                StringWriter w(s);
                w.Write(L"Update ");
                this->WriteItemKey(w, prev);
                w.Write(L" <- ");
                this->WriteItem(w, next);
                Trace.WriteInfo(Constants::StoreSource, "{0}", s);

                auto updateResult = store_.Update(
                    transaction,
                    prev.Type,
                    prev.Key,
                    checkSequenceNumber,
                    next.Key,
                    next.Value.data(),
                    next.Value.size());

                CODING_ERROR_ASSERT(updateResult.IsSuccess() == expectSuccess);

                if (expectSuccess)
                {
                    CODING_ERROR_ASSERT(transaction->Commit().IsSuccess() == expectSuccess);
                }
                else
                {
                    transaction.reset();
                }
            }
        }

        void BulkDelete(
            ITableGenerator const & generator,
            IndexRange const & range) const
        {
            this->BulkDelete(generator, range, 0, true);
        }

        void BulkDelete(
            ITableGenerator const & generator,
            IndexRange const & range,
            _int64 checkSequenceNumber,
            bool expectSuccess) const
        {
            ILocalStore::TransactionSPtr transaction;
            CODING_ERROR_ASSERT(store_.CreateTransaction(transaction).IsSuccess());

            StoreItem item;
            generator.GetType(/*out*/item.Type);

            for (int i = range.Start; i<range.Limit; i += range.Step)
            {
                generator.GetKey(i, /*out*/item.Key);
                this->DumpItemKey(L"Delete ", item);

                auto deleteResult = store_.Delete(
                    transaction,
                    item.Type,
                    item.Key,
                    checkSequenceNumber);

                CODING_ERROR_ASSERT(deleteResult.IsSuccess() == expectSuccess);
            }

            CODING_ERROR_ASSERT(transaction->Commit().IsSuccess());
        }

        void BulkValidate(ITableGenerator const & generator, IndexRange const & range) const
        {
            ILocalStore::TransactionSPtr transaction;
            CODING_ERROR_ASSERT(store_.CreateTransaction(transaction).IsSuccess());

            StoreItem actual;
            StoreItem expected;

            generator.GetType(/*out*/expected.Type);
            for (int i = range.Start; i<range.Limit; i += range.Step)
            {
                generator.GetKey(i, /*out*/expected.Key);
                generator.GetValue(i, /*out*/expected.Value);

                ILocalStore::EnumerationSPtr enumeration;
                CODING_ERROR_ASSERT(store_.CreateEnumerationByTypeAndKey(transaction, expected.Type, expected.Key, enumeration).IsSuccess());
                CODING_ERROR_ASSERT(enumeration->MoveNext().IsSuccess());

                enumeration->CurrentType(actual.Type);
                enumeration->CurrentKey(actual.Key);
                enumeration->CurrentValue(actual.Value);

                this->DumpItem(L"Validate ", expected);

                CODING_ERROR_ASSERT(actual.Type == expected.Type);
                CODING_ERROR_ASSERT(actual.Key == expected.Key);
                CODING_ERROR_ASSERT(actual.Value == expected.Value);
            }

            CODING_ERROR_ASSERT(transaction->Commit().IsSuccess());
        }

        void BulkValidateEnum(
            ILocalStore::EnumerationSPtr const & enumeration,
            ITableGenerator const & generator,
            IndexRange const & range) const
        {
            StoreItem actual;
            StoreItem expected;

            generator.GetType(/*out*/expected.Type);
            for (int i = range.Start; i<range.Limit; i += range.Step)
            {
                generator.GetKey(i, /*out*/expected.Key);
                generator.GetValue(i, /*out*/expected.Value);

                CODING_ERROR_ASSERT(enumeration->MoveNext().IsSuccess());

                enumeration->CurrentType(actual.Type);
                enumeration->CurrentKey(actual.Key);
                enumeration->CurrentValue(actual.Value);

                this->DumpItem(L"ValidateEnum expected ", expected);
                this->DumpItem(L"ValidateEnum actual   ", actual);

                CODING_ERROR_ASSERT(actual.Type == expected.Type);
                CODING_ERROR_ASSERT(actual.Key == expected.Key);
                CODING_ERROR_ASSERT(actual.Value == expected.Value);
            }
        }

        void DumpTable(ILocalStore::TransactionSPtr const & transaction, wstring const & type) const
        {
            Trace.WriteInfo(Constants::StoreSource, "DumpTable:");

            ILocalStore::EnumerationSPtr enumeration;

            CODING_ERROR_ASSERT(store_.CreateEnumerationByTypeAndKey(transaction, type, L"", enumeration).IsSuccess());

            StoreItem item;

            while (enumeration->MoveNext().IsSuccess())
            {
                enumeration->CurrentType(item.Type);
                enumeration->CurrentKey(item.Key);
                enumeration->CurrentValue(item.Value);

                this->DumpItem(L"    ", item);
            }
        }

        void DumpItemKey(wchar_t const * prefix, StoreItem const & item) const
        {
            wstring s;
            StringWriter w(s);
            if (prefix) { w.Write(prefix); }
            this->WriteItemKey(w, item);
            Trace.WriteInfo(Constants::StoreSource, "{0}", s);
        }

        void DumpItem(wchar_t const * prefix, StoreItem const & item) const
        {
            wstring s;
            StringWriter w(s);
            if (prefix) { w.Write(prefix); }
            this->WriteItem(w, item);
            Trace.WriteInfo(Constants::StoreSource, "{0}", s);
        }

        void WriteItemKey(StringWriter & w, StoreItem const & item) const
        {
            w.Write(item.Type);
            w.Write(L"[");
            w.Write(item.Key);
            w.Write(L"]");
        }

        void WriteItem(StringWriter & w, StoreItem const & item) const
        {
            const size_t valueEllipses = 32;
            this->WriteItemKey(w, item);

            w.Write(L" = {");
            size_t limit = min(valueEllipses, item.Value.size());
            for (size_t i = 0; i<limit; i++)
            {
                if (i > 0) { w.Write(L","); }
                w.Write(static_cast<int>(item.Value[i]));
            }
            if (item.Value.size() > valueEllipses) { w.Write(L", ..."); }
            w.Write("}");
        }

    private:
        static const _int64 OperationNumberAtInsert = 10;
        ILocalStore & store_;
    };

    class IdentityGenerator : public ITableGenerator
    {
        DENY_COPY(IdentityGenerator);
    public:
        IdentityGenerator() : type_(L"Integer") { }
        virtual void GetType(/*out*/wstring & type) const { type = type_; }
        virtual void GetKey(int index, /*out*/wstring & key) const
        {
            key.clear();
            _int64 n = static_cast<_int64>(index);
            StringWriter(key).Write("{0}", n);
        }
        virtual void GetValue(int index, /*out*/vector<unsigned _int8> & value) const
        {
            value.clear();
            auto p = reinterpret_cast<unsigned _int8 *>(&index);
            for (int i = 0; i<sizeof(index); i++) { value.push_back(p[i]); }
        }
    private:
        const wstring type_;
    };

    class Plus5Generator : public ITableGenerator
    {
        DENY_COPY(Plus5Generator);
    public:
        Plus5Generator() : type_(L"Integer") { }
        virtual void GetType(/*out*/wstring & type) const { type = type_; }
        virtual void GetKey(int index, /*out*/wstring & key) const
        {
            key.clear();
            _int64 n = static_cast<_int64>(index);
            StringWriter(key).Write("{0}", n);
        }
        virtual void GetValue(int index, /*out*/vector<unsigned _int8> & value) const
        {
            value.clear();
            auto p = reinterpret_cast<unsigned _int8 *>(&index);
            for (int i = 0; i<sizeof(index) + 5; i++) { value.push_back(p[i%sizeof(index)]); }
        }
    private:
        const wstring type_;
    };

    class LargeKeyValueGenerator : public ITableGenerator
    {
        DENY_COPY(LargeKeyValueGenerator);
    public:
        LargeKeyValueGenerator(size_t keySize, size_t valueSize)
            : type_(L"String"), keySize_(keySize), valueSize_(valueSize)
        { }
        virtual void GetType(/*out*/wstring & type) const { type = type_; }
        virtual void GetKey(int index, /*out*/wstring & key) const
        {
            key.clear();
            key.resize(static_cast<size_t>(keySize_));

            auto p = const_cast<wchar_t *>(key.data());
            const wchar_t chars[] =
                L"ABCEDFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-=~!@#$%^&*()_+[]{}\\|;:/?,.<>`'";
            const int charsCount = sizeof(chars) / sizeof(wchar_t);

            for (size_t i = 0; i<keySize_; i++)
            {
                p[i] = chars[(index + i) % charsCount];
            }
        }
        virtual void GetValue(int index, /*out*/vector<unsigned _int8> & value) const
        {
            value.clear();
            value.resize(static_cast<size_t>(valueSize_));

            auto p = const_cast<unsigned _int8 *>(value.data());
            for (size_t i = 0; i<valueSize_; i++)
            {
                p[i] = static_cast<unsigned _int8>(((index + i) ^ (i >> 8) ^ (i >> 16)) & 0xff);
            }
        }
    private:
        const wstring type_;
        const size_t keySize_;
        const size_t valueSize_;
    };

    /// <summary>
    /// Wrapper class that invokes ESE backup's 'truncate' option in an async operation.
    /// <summary>
    class BackupTruncate1AsyncOperation : public Common::AsyncOperation
    {
        DENY_COPY(BackupTruncate1AsyncOperation);

    public:
        BackupTruncate1AsyncOperation(
            __in std::wstring const & instancePrefix,
            __in std::wstring const & baseDir,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent, true)
            , instancePrefix_(instancePrefix)
            , baseDir_(baseDir)
        {
        }

        virtual ~BackupTruncate1AsyncOperation()
        {            
        }

    protected:
        void OnStart(Common::AsyncOperationSPtr const & proxySPtr)
        {
            Common::Threadpool::Post([this, proxySPtr]()
            {
                DoBackup(proxySPtr);
            });
        }

    private:
        shared_ptr<EseLocalStore> CreateTestLocalStore(std::wstring const & directory)
        {
            EseLocalStoreSettings settings(L"FabricLocalstore.edb", L"");
            // scenario-wise, truncate is mainly used when incremental backup is turned on
            settings.IsIncrementalBackupEnabled = true;
            settings.MaxDefragFrequency = TimeSpan::Zero;
            return make_shared<EseLocalStore>(directory, settings, 0x0);
        }

        void DeleteBaseDirectory()
        {            
            ErrorCode error;

            for (auto retry=0; retry<10; ++retry)
            {
                if (!Directory::Exists(baseDir_)) { return; }

                error = Directory::Delete(baseDir_, true);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(TraceType, "Deleted '{0}'...", baseDir_);

                    return;
                }
                else
                {
                    Trace.WriteInfo(TraceType, "Retrying cleanup of '{0}'...", baseDir_);
                    Sleep(1000);
                }
            }

            CODING_ERROR_ASSERT(error.IsSuccess());
        }

        void DoBackup(Common::AsyncOperationSPtr const & operation)
        {
            {
                DeleteBaseDirectory();

                Trace.WriteInfo(TraceType, "Creating database {0}", instancePrefix_);
                auto store = CreateTestLocalStore(baseDir_);
                ErrorCode error = store->Initialize(L"TestEseLocalStore" + instancePrefix_);
                CODING_ERROR_ASSERT(error.IsSuccess());

                IdentityGenerator identityGenerator;
                const TableValidator validator(*store);

                // To test if log files are cleaned up after a full backup, set these to higher values.
                // After the log files hit a certain size limit on disk (~64 MB), they are cleaned up. 
                // E.g 
                // with MaxIterations = 1000 and ItemCount = 100000, clean up would happen after about 1100
                // files are created, each of 64 kb size in the database folder. After clean up, about 600
                // files would still remain. i.e. 500 of the oldest log files are removed.
                // 
                const int MaxIterations = 25;
                const int ItemCount = 1000;

                for (int i = 0; i < MaxIterations; i++)
                {
                    IndexRange creationRange(ItemCount * i, 1, ItemCount * i + ItemCount);
                    IndexRange verificationRange(0, 1, ItemCount * i + ItemCount);

                    Trace.WriteInfo(
                        TraceType,
                        "{0}/{1}. Populating database {2} with generated data. Range: {3} - {4}",
                        i + 1,
                        MaxIterations,
                        instancePrefix_,
                        creationRange.Start,
                        creationRange.Limit);
                    validator.BulkInsert(identityGenerator, creationRange);

                    Trace.WriteInfo(TraceType, "Validating contents against generated data for database {0}", instancePrefix_);
                    validator.BulkValidate(identityGenerator, verificationRange);

                    Trace.WriteInfo(TraceType, "Truncating logs of existing database {0}", instancePrefix_);

                    ErrorCode backupResult = store->Backup(L"");
                    CODING_ERROR_ASSERT(error.IsSuccess());
                }

                TryComplete(operation, error);
            }

            // delete dir outside scope of create so that ESE related files are not used anymore.
            DeleteBaseDirectory();
        };

        std::wstring baseDir_;
        std::wstring instancePrefix_;
    };

    // ------------------------------------------------------------------------

    BOOST_FIXTURE_TEST_SUITE(TestEseLocalStoreSuite, TestEseLocalStore)

    BOOST_AUTO_TEST_CASE(EnableOverwriteOnUpdate)
    {
        StoreConfig::GetConfig().EnableOverwriteOnUpdate = true;
        const TemporaryCurrentDirectory directory(std::wstring(L"LocalStore-EnableOverwriteOnUpdate"));
        const wstring dbFileName = L"FabricLocalstore.edb";

        auto dbFilePath = Path::Combine(directory.Path, dbFileName);

        Common::StringLiteral testTraceType = "EnableOverwriteOnUpdate";

        Trace.WriteInfo(testTraceType, "Database path:{0}", directory.Path.c_str());

        wstring type = L"TestType";
        wstring key = L"TestKey";
        vector<byte> value(1024 * 1024);

        const _int64 lsn = ILocalStore::OperationNumberUnspecified;
        const _int64 seqNum = ILocalStore::SequenceNumberIgnore;

        {
            Trace.WriteInfo(testTraceType, "Creating default local database.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            
            Trace.WriteInfo(testTraceType, "Inserting key...");

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());
            CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn).IsSuccess());
            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }
        
        int64 initialDbSize;
        CODING_ERROR_ASSERT(File::GetSize(dbFilePath, initialDbSize).IsSuccess());

        {
            Trace.WriteInfo(testTraceType, "Reopening local database...");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr readTxn;
            CODING_ERROR_ASSERT(store->CreateTransaction(readTxn).IsSuccess());

            ILocalStore::EnumerationSPtr en;
            CODING_ERROR_ASSERT(store->CreateEnumerationByTypeAndKey(readTxn, type, key, en).IsSuccess());

            Trace.WriteInfo(testTraceType, "Reading the key and keeping txn open...");

            while (en->MoveNext().IsSuccess())
            {
                wstring key1;
                ByteBuffer value1;

                en->CurrentKey(key1);
                en->CurrentValue(value1);
            }                      

            Trace.WriteInfo(testTraceType, "Updating existing key in loop...");
            for (_int64 i = 1; i <= 100; i++)
            {
                ILocalStore::TransactionSPtr updateTxn;
                CODING_ERROR_ASSERT(store->CreateTransaction(updateTxn).IsSuccess());
                CODING_ERROR_ASSERT(store->Update(updateTxn, type, key, seqNum, key, value.data(), value.size(), lsn).IsSuccess());
                CODING_ERROR_ASSERT(updateTxn->Commit().IsSuccess());
            }

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        int64 finalDbSize1;
        CODING_ERROR_ASSERT(File::GetSize(dbFilePath, finalDbSize1).IsSuccess());
        
        CODING_ERROR_ASSERT(initialDbSize == finalDbSize1);

        StoreConfig::GetConfig().EnableOverwriteOnUpdate = false;

        {
            Trace.WriteInfo(testTraceType, "Reopening local database...");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr readTxn;
            CODING_ERROR_ASSERT(store->CreateTransaction(readTxn).IsSuccess());

            ILocalStore::EnumerationSPtr en;
            CODING_ERROR_ASSERT(store->CreateEnumerationByTypeAndKey(readTxn, type, key, en).IsSuccess());

            Trace.WriteInfo(testTraceType, "Reading the key and keeping txn open...");

            while (en->MoveNext().IsSuccess())
            {
                wstring key1;
                ByteBuffer value1;

                en->CurrentKey(key1);
                en->CurrentValue(value1);
            }

            Trace.WriteInfo(testTraceType, "Updating existing key in loop...");
            for (_int64 i = 1; i <= 50; i++)
            {
                ILocalStore::TransactionSPtr updateTxn;
                CODING_ERROR_ASSERT(store->CreateTransaction(updateTxn).IsSuccess());
                CODING_ERROR_ASSERT(store->Update(updateTxn, type, key, seqNum, key, value.data(), value.size(), lsn).IsSuccess());
                CODING_ERROR_ASSERT(updateTxn->Commit().IsSuccess());
            }

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        int64 finalDbSize2;
        CODING_ERROR_ASSERT(File::GetSize(dbFilePath, finalDbSize2).IsSuccess());

        CODING_ERROR_ASSERT(finalDbSize2 >= finalDbSize1 + 50 * 1024 * 1024);

        StoreConfig::GetConfig().Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(InsertEnumerateDelete)
    {
        TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-InsertEnumerateDelete"));
        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;
        IndexRange range(1000);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());
        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
        {
            Trace.WriteInfo(TraceType, "Reopening the database for update.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Updating all items in database.");
            validator.BulkUpdate(identityGenerator, plus5Generator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(plus5Generator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
        {
            Trace.WriteInfo(TraceType, "Reopening the database for delete.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(plus5Generator, range);

            Trace.WriteInfo(TraceType, "Deleting all items from database.");
            validator.BulkDelete(plus5Generator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
    }

    BOOST_AUTO_TEST_CASE(InsertLargeKeys)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-InsertLargeKeys"));

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            // Account for inflation of index length based on ESE's use of LCMapString
            // NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY
            //
            size_t maxIndex = (JET_cbKeyMost8KBytePage - 256);

            {
                StoreItem item;

                wstring type(1, 'T');
                wstring key(maxIndex / sizeof(wchar_t), 'K');

                vector<byte> value;
                value.push_back(0);

                item.Type = move(type);
                item.Key = move(key);
                item.Value = move(value);

                ILocalStore::TransactionSPtr tx;
                auto error = store->CreateTransaction(tx);
                CODING_ERROR_ASSERT(error.IsSuccess());

                error = store->Insert(tx, item.Type, item.Key, item.Value.data(), item.Value.size());
                CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::Success));

                CODING_ERROR_ASSERT(tx->Commit().IsSuccess());
            }

            {
                StoreItem item;

                wstring type(1, 'T');
                wstring key(maxIndex, 'K');

                vector<byte> value;
                value.push_back(0);

                item.Type = move(type);
                item.Key = move(key);
                item.Value = move(value);

                ILocalStore::TransactionSPtr tx;
                auto error = store->CreateTransaction(tx);
                CODING_ERROR_ASSERT(error.IsSuccess());

                error = store->Insert(tx, item.Type, item.Key, item.Value.data(), item.Value.size());

                CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::StoreKeyTooLarge));

                tx->Rollback();
            }

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
    }

    BOOST_AUTO_TEST_CASE(InsertLargeValue)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-InsertLargeValue"));
        const IndexRange range(0, 1);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            const TableValidator validator(*store);

            size_t maxIndex = (JET_cbKeyMost8KBytePage - 256);

            const LargeKeyValueGenerator largeGenerator(maxIndex / sizeof(wchar_t), 10 * 1024 * 1024);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
    }

    BOOST_AUTO_TEST_CASE(DatabasePageSizeConflict)
    {
        Config config;
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-DatabasePageSizeConflict"));

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        size_t maxIndex = (JET_cbKeyMost4KBytePage - 140);

        {
            StoreConfig::GetConfig().DatabasePageSizeInKB = 4;

            Trace.WriteInfo(TraceType, "Creating 4KB page size database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const IndexRange range(0, 1);
            const TableValidator validator(*store);
            const LargeKeyValueGenerator largeGenerator(maxIndex / sizeof(wchar_t), 1 * 1024);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        {
            StoreConfig::GetConfig().DatabasePageSizeInKB = 8;

            Trace.WriteInfo(TraceType, "Creating 8KB page size database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const IndexRange range(2, 3);
            const TableValidator validator(*store);
            const LargeKeyValueGenerator largeGenerator(maxIndex / sizeof(wchar_t), 1 * 1024);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        StoreConfig::GetConfig().Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(DatabaseLogFileSizeConflict)
    {
        Config config;
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-DatabaseLogFileSizeConflict"));

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        size_t maxIndex = (JET_cbKeyMost4KBytePage - 140);

        {
            StoreConfig::GetConfig().EseLogFileSizeInKB = 5120;

            Trace.WriteInfo(TraceType, "Creating 5MB logfile size database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const IndexRange range(0, 1);
            const TableValidator validator(*store);
            const LargeKeyValueGenerator largeGenerator(maxIndex / sizeof(wchar_t), 1 * 1024);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        {
            StoreConfig::GetConfig().EseLogFileSizeInKB = 1024;

            Trace.WriteInfo(TraceType, "Creating 1MB logfile size database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const IndexRange range(2, 3);
            const TableValidator validator(*store);
            const LargeKeyValueGenerator largeGenerator(maxIndex / sizeof(wchar_t), 1 * 1024);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(largeGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        StoreConfig::GetConfig().Test_Reset();
    }
    
    BOOST_AUTO_TEST_CASE(SequenceNumberCheck)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-SequenceNumberCheck"));
        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;
        IndexRange range(100);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range);
            validator.BulkValidate(identityGenerator, range);

            for (int i = 1; i<10; i++)
            {
                Trace.WriteInfo(TraceType, "Attempting to update at too high version.");
                validator.BulkUpdate(identityGenerator, plus5Generator, range, CheckSequenceType::HigherThanCurrentSN, false);

                Trace.WriteInfo(TraceType, "Updating at correct version.");
                validator.BulkUpdate(identityGenerator, identityGenerator, range, CheckSequenceType::MatchCurrentSN, true);

                Trace.WriteInfo(TraceType, "Attempting to update at too low version.");
                validator.BulkUpdate(identityGenerator, plus5Generator, range, CheckSequenceType::LowerThanCurrentSN, false);

                Trace.WriteInfo(TraceType, "Validating that only the correct change occurred.");
                validator.BulkValidate(identityGenerator, range);
            }

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range);

            validator.BulkDelete(identityGenerator, range, 20, false);
            validator.BulkDelete(identityGenerator, range, 21, false);

            Trace.WriteInfo(TraceType, "Closing the database.");
        }
    }

    BOOST_AUTO_TEST_CASE(SequenceNumberCheck2)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-SequenceNumberCheck2"));
        const IdentityGenerator identityGenerator;
        IndexRange range(1);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());
        {
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            const TableValidator validator(*store);

            validator.BulkInsert(identityGenerator, range);

            StoreItem item;
            identityGenerator.GetType(item.Type);
            identityGenerator.GetKey(0, item.Key);
            identityGenerator.GetValue(0, item.Value);

            // Initialize LSN
            //
            ILocalStore::TransactionSPtr tx0;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx0).IsSuccess());

            BYTE array0[] = { 0x00 };
            auto updateResult0 = store->Update(
                tx0, item.Type, item.Key, 0, item.Key, &array0[0], sizeof(array0));
            CODING_ERROR_ASSERT(updateResult0.IsSuccess());

            updateResult0 = store->UpdateOperationLSN(
                tx0, item.Type, item.Key, 1);
            CODING_ERROR_ASSERT(updateResult0.IsSuccess());

            CODING_ERROR_ASSERT(tx0->Commit().IsSuccess());

            // Concurrent LSN update
            //
            ILocalStore::TransactionSPtr tx1;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx1).IsSuccess());

            ILocalStore::TransactionSPtr tx2;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx2).IsSuccess());

            ILocalStore::TransactionSPtr tx3;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx3).IsSuccess());

            BYTE array1[] = { 0x01, 0x02 };
            auto updateResult1 = store->Update(
                tx1, item.Type, item.Key, 1, item.Key, &array1[0], sizeof(array1));
            CODING_ERROR_ASSERT(updateResult1.IsSuccess());

            updateResult1 = store->UpdateOperationLSN(
                tx1, item.Type, item.Key, 2);
            CODING_ERROR_ASSERT(updateResult1.IsSuccess());

            CODING_ERROR_ASSERT(tx1->Commit().IsSuccess());

            tx1.reset();

            // Using the old LSN results in a write conflict
            //
            BYTE array2[] = { 0x03, 0x04, 0x05 };
            auto updateResult2 = store->Update(
                tx2, item.Type, item.Key, 1, item.Key, &array2[0], sizeof(array2));
            CODING_ERROR_ASSERT(updateResult2.IsError(ErrorCodeValue::StoreWriteConflict));

            CODING_ERROR_ASSERT(!tx2->Commit().IsSuccess());

            // Using the new LSN results in a sequence number check error
            //
            BYTE array3[] = { 0x06, 0x07, 0x08 };
            auto updateResult3 = store->Update(
                tx3, item.Type, item.Key, 2, item.Key, &array3[0], sizeof(array3));
            CODING_ERROR_ASSERT(updateResult3.IsError(ErrorCodeValue::StoreSequenceNumberCheckFailed));

            CODING_ERROR_ASSERT(!tx3->Commit().IsSuccess());
        }
    }

    BOOST_AUTO_TEST_CASE(WriteConflict)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-WriteConflict"));
        const IdentityGenerator identityGenerator;
        IndexRange range(1);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            const TableValidator validator(*store);

            validator.BulkInsert(identityGenerator, range);

            StoreItem item;
            identityGenerator.GetType(item.Type);
            identityGenerator.GetKey(0, item.Key);
            identityGenerator.GetValue(0, item.Value);

            ILocalStore::TransactionSPtr tx1;
            ILocalStore::TransactionSPtr tx2;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx1).IsSuccess());
            CODING_ERROR_ASSERT(store->CreateTransaction(tx2).IsSuccess());

            BYTE array1[] = { 0x01, 0x02 };
            auto updateResult1 = store->Update(
                tx1, item.Type, item.Key, 0, item.Key, &array1[0], sizeof(array1));
            CODING_ERROR_ASSERT(updateResult1.IsSuccess());

            BYTE array2[] = { 0x03, 0x04, 0x05 };
            auto updateResult2 = store->Update(
                tx2, item.Type, item.Key, 0, item.Key, &array2[0], sizeof(array2));
            CODING_ERROR_ASSERT(!updateResult2.IsSuccess());

            CODING_ERROR_ASSERT(tx1->Commit().IsSuccess());
            CODING_ERROR_ASSERT(!tx2->Commit().IsSuccess());
        }
    }

    BOOST_AUTO_TEST_CASE(InsertReadDelete)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-InsertDeleteTest"));

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            auto store = CreateTestLocalStore(directory);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr tx;
            CODING_ERROR_ASSERT(store->CreateTransaction(tx).IsSuccess());

            wstring type(L"MyType");
            wstring key(L"MyKey");

            BYTE data[] = { 0xaf };
            auto error = store->Insert(tx, type, key, &data[0], sizeof(data));
            CODING_ERROR_ASSERT(error.IsSuccess());

            ILocalStore::EnumerationSPtr enumerator;
            error = store->CreateEnumerationByTypeAndKey(tx, type, key, enumerator);
            CODING_ERROR_ASSERT(error.IsSuccess());

            error = enumerator->MoveNext();
            CODING_ERROR_ASSERT(error.IsSuccess());

            //
            // Read insert in same Tx
            //
            wstring result;

            error = enumerator->CurrentType(result);
            CODING_ERROR_ASSERT(error.IsSuccess());
            CODING_ERROR_ASSERT(result == type);

            error = enumerator->CurrentKey(result);
            CODING_ERROR_ASSERT(error.IsSuccess());
            CODING_ERROR_ASSERT(result == key);

            error = enumerator->MoveNext();
            CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::EnumerationCompleted));

            //
            // Delete insert in same Tx
            //
            error = store->Delete(tx, type, key);
            CODING_ERROR_ASSERT(error.IsSuccess());

            error = tx->Commit();
            CODING_ERROR_ASSERT(error.IsSuccess());
        }
    }

    // Test that outstanding enumerations can destruct after their associated
    // transactions without causing asserts. This test case intentionally triggers
    // code paths that will hit test asserts, so it should only run if
    // test asserts are disabled.
    //
    BOOST_AUTO_TEST_CASE(EnumerationLifetime)
    {
        if (Assert::IsTestAssertEnabled())
        {
            Trace.WriteInfo(TraceType, "Test asserts enabled: skipping EnumerationLifetime test");
            return;
        }

        const TemporaryCurrentDirectory directory1(std::wstring(L"FabricTest-LocalStore-EnumerationLifetime1"));
        const TemporaryCurrentDirectory directory2(std::wstring(L"FabricTest-LocalStore-EnumerationLifetime2"));
        const IdentityGenerator identityGenerator;
        IndexRange range(1);

        Trace.WriteInfo(TraceType, "Database path:{0}", directory1.Path.c_str());

        // single enumeration by key
        {
            ErrorCode error(ErrorCodeValue::Success);

            auto store = CreateTestLocalStore(directory1);
            error = store->Initialize(TestStoreInstanceName);
            VERIFY(error.IsSuccess(), "Initialize: error = {0}", error);

            weak_ptr<IStoreBase::TransactionBase> wtx1;
            ILocalStore::EnumerationSPtr enum1;
            {
                ILocalStore::TransactionSPtr tx1;
                error = store->CreateTransaction(tx1);
                VERIFY(error.IsSuccess(), "CreateTransaction: error = {0}", error);
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                error = store->CreateEnumerationByTypeAndKey(tx1, L"", L"", enum1);
                VERIFY(error.IsSuccess(), "CreateEnumerationByTypeAndKey: error = {0}", error);
                VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                wtx1 = tx1;
            }

            VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());

            enum1.reset();

            VERIFY(enum1.use_count() == 0, "enum1.use_count = {0} != 0", enum1.use_count());
            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());
        }

        Trace.WriteInfo(TraceType, "Database path:{0}", directory2.Path.c_str());

        // single enumeration by LSN
        {
            ErrorCode error(ErrorCodeValue::Success);

            auto store = CreateTestLocalStore(directory2, EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN);
            error = store->Initialize(TestStoreInstanceName);
            VERIFY(error.IsSuccess(), "Initialize: error = {0}", error);

            weak_ptr<IStoreBase::TransactionBase> wtx1;
            ILocalStore::EnumerationSPtr enum1;
            {
                ILocalStore::TransactionSPtr tx1;
                error = store->CreateTransaction(tx1);
                VERIFY(error.IsSuccess(), "CreateTransaction: error = {0}", error);
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                error = store->CreateEnumerationByOperationLSN(tx1, 0, enum1);
                VERIFY(error.IsSuccess(), "CreateEnumerationByOperationLSN: error = {0}", error);
                VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                wtx1 = tx1;
            }

            VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());

            enum1.reset();

            VERIFY(enum1.use_count() == 0, "enum1.use_count = {0} != 0", enum1.use_count());
            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());
        }

        // multiple enumerations and explicitly rollback tx
        {
            ErrorCode error(ErrorCodeValue::Success);

            auto store = CreateTestLocalStore(directory2, EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN);
            error = store->Initialize(TestStoreInstanceName);
            VERIFY(error.IsSuccess(), "Initialize: error = {0}", error);

            weak_ptr<IStoreBase::TransactionBase> wtx1;
            ILocalStore::TransactionSPtr tx2;
            ILocalStore::EnumerationSPtr enum1;
            ILocalStore::EnumerationSPtr enum2;
            {
                ILocalStore::TransactionSPtr tx1;
                error = store->CreateTransaction(tx1);
                VERIFY(error.IsSuccess(), "CreateTransaction: error = {0}", error);
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                error = store->CreateEnumerationByTypeAndKey(tx1, L"", L"", enum1);
                VERIFY(error.IsSuccess(), "CreateEnumerationByTypeAndKey: error = {0}", error);
                VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                error = store->CreateEnumerationByOperationLSN(tx1, 0, enum2);
                VERIFY(error.IsSuccess(), "CreateEnumerationByOperationLSN: error = {0}", error);
                VERIFY(enum2.use_count() == 1, "enum2.use_count = {0} != 1", enum2.use_count());
                VERIFY(tx1.use_count() == 1, "tx1.use_count = {0} != 1", tx1.use_count());

                wtx1 = tx1;
                tx2 = tx1;
            }

            VERIFY(enum1.use_count() == 1, "enum1.use_count = {0} != 1", enum1.use_count());
            VERIFY(enum2.use_count() == 1, "enum2.use_count = {0} != 1", enum2.use_count());
            VERIFY(wtx1.use_count() == 1, "wtx1.use_count = {0} != 1", wtx1.use_count());
            VERIFY(tx2.use_count() == 1, "tx2.use_count = {0} != 1", tx2.use_count());

            tx2->Rollback();

            VERIFY(tx2.use_count() == 1, "tx2.use_count = {0} != 1", tx2.use_count());
            VERIFY(wtx1.use_count() == 1, "wtx1.use_count = {0} != 1", wtx1.use_count());

            error = enum1->MoveNext();
            VERIFY(error.IsError(ErrorCodeValue::StoreTransactionNotActive), "enum1->MoveNext() = {0}", error);

            error = enum2->MoveNext();
            VERIFY(error.IsError(ErrorCodeValue::StoreTransactionNotActive), "enum2->MoveNext() = {0}", error);

            tx2.reset();

            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());

            error = enum1->MoveNext();
            VERIFY(error.IsError(ErrorCodeValue::StoreTransactionNotActive), "enum1->MoveNext() = {0}", error);

            error = enum2->MoveNext();
            VERIFY(error.IsError(ErrorCodeValue::StoreTransactionNotActive), "enum2->MoveNext() = {0}", error);

            enum1.reset();

            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());

            enum2.reset();

            VERIFY(wtx1.use_count() == 0, "wtx1.use_count = {0} != 0", wtx1.use_count());
        }
    }

    BOOST_AUTO_TEST_CASE(OperationLSNFlag)
    {
        Config config;
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-OperationLSNFlag"));

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating default local database.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(TraceType, "Loading local database with OperationLSN column enabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsError(ErrorCodeValue::StoreUnexpectedError));

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(TraceType, "Loading local database with OperationLSN column disabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            Trace.WriteInfo(TraceType, "Closing the database.");
        }

        StoreConfig::GetConfig().Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(LastModifiedOnPrimaryFlag)
    {
        Config config;
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-LastModifiedOnPrimaryFlag"));

        Common::StringLiteral testTraceType = "LastModifiedOnPrimaryFlag";

        Trace.WriteInfo(testTraceType, "Database path:{0}", directory.Path.c_str());

        wstring type = L"type";
        wstring keyPrefix = L"Key";
        vector<byte> value(10);
        DateTime lastModifiedOnPrimary = DateTime::Now();
        const _int64 lsn = ILocalStore::OperationNumberUnspecified;
        const _int64 seqNum = ILocalStore::SequenceNumberIgnore;

        map<wstring, _int64> keyFileTimeMap;
        
        {
            Trace.WriteInfo(testTraceType, "Creating default local database.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            Trace.WriteInfo(testTraceType, "Inserting values (key: 1-10).");
            for (_int64 i = 1; i <= 10 ; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);                
                CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn).IsSuccess());
                keyFileTimeMap.insert(make_pair(key, 0));
            }

            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(testTraceType, "Loading local database with LastModifiedOnPrimary column enabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            Trace.WriteInfo(testTraceType, "Inserting values with last modified on primary (key: 11-20).");
            for (_int64 i = 11; i <= 20; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
				FILETIME fileTime = lastModifiedOnPrimary.AsFileTime;
                CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn, &fileTime).IsSuccess());
                keyFileTimeMap.insert(make_pair(key, lastModifiedOnPrimary.AsLargeInteger.QuadPart));
            }

            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(testTraceType, "Loading local database with LastModifiedOnPrimary column disabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            Trace.WriteInfo(testTraceType, "Inserting values (key: 21- 30).");
            for (_int64 i = 21; i <= 30; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
                CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn).IsSuccess());
                keyFileTimeMap.insert(make_pair(key, 0));
            }

            Trace.WriteInfo(testTraceType, "updating existing values (key: 1-20).");
            for (_int64 i = 1; i <= 20; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
                CODING_ERROR_ASSERT(store->Update(txn, type, key, seqNum, key, value.data(), value.size(), lsn).IsSuccess());
            }

            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(testTraceType, "Loading local database with LastModifiedOnPrimary column enabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            ILocalStore::EnumerationSPtr en;
            CODING_ERROR_ASSERT(store->CreateEnumerationByTypeAndKey(txn, type, keyPrefix, en).IsSuccess());

            Trace.WriteInfo(testTraceType, "Verifying lastModifiedOnPrimary for all items.");

            while (en->MoveNext().IsSuccess())
            {
                wstring key;
                FILETIME filetime;

                en->CurrentKey(key);
                en->CurrentLastModifiedOnPrimaryFILETIME(filetime);

                _int64 filetimeInt64 = static_cast<_int64>(DateTime(filetime).AsLargeInteger.QuadPart);

                Trace.WriteInfo(
                    testTraceType,
                    "Key: ({0}) File-time: (Expected: {1}, Actual: {2})", 
                    key,
                    keyFileTimeMap.at(key),
                    filetimeInt64);

                CODING_ERROR_ASSERT(keyFileTimeMap.at(key) == filetimeInt64);

                keyFileTimeMap.erase(key);
            }

            CODING_ERROR_ASSERT(keyFileTimeMap.empty());
            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        StoreConfig::GetConfig().Test_Reset();
    }

    BOOST_AUTO_TEST_CASE(LastModifiedOnPrimaryFlag1)
    {
        Config config;
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-LastModifiedOnPrimaryFlag1"));

        Common::StringLiteral testTraceType = "LastModifiedOnPrimaryFlag1";

        Trace.WriteInfo(testTraceType, "Database path:{0}", directory.Path.c_str());

        wstring type = L"type";
        wstring keyPrefix = L"Key";
        vector<byte> value(10);
        DateTime lastModifiedOnPrimary = DateTime::Now();
        const _int64 lsn = ILocalStore::OperationNumberUnspecified;
        const _int64 seqNum = ILocalStore::SequenceNumberIgnore;

        map<wstring, _int64> keyFileTimeMap;

        {
            Trace.WriteInfo(testTraceType, "Creating default local database.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            Trace.WriteInfo(testTraceType, "Inserting values (key: 1-10).");
            for (_int64 i = 1; i <= 10; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
				FILETIME fileTime = lastModifiedOnPrimary.AsFileTime;
                CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn, &fileTime).IsSuccess());
                keyFileTimeMap.insert(make_pair(key, lastModifiedOnPrimary.AsLargeInteger.QuadPart));
            }

            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(testTraceType, "Loading local database with LastModifiedOnPrimary column enabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());
            
            Trace.WriteInfo(testTraceType, "Inserting values with last modified on primary (key: 11-20).");
            for (_int64 i = 11; i <= 20; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
                CODING_ERROR_ASSERT(store->Insert(txn, type, key, value.data(), value.size(), lsn).IsSuccess());
                keyFileTimeMap.insert(make_pair(key, 0));
            }

            Trace.WriteInfo(testTraceType, "updating existing values (key: 1-10).");
            for (_int64 i = 1; i <= 10; i++)
            {
                wstring key = keyPrefix + std::to_wstring(i);
                CODING_ERROR_ASSERT(store->Update(txn, type, key, seqNum, key, value.data(), value.size(), lsn).IsSuccess());
            }

            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        {
            Trace.WriteInfo(testTraceType, "Loading local database with LastModifiedOnPrimary column enabled.");
            auto store = CreateTestLocalStore(directory, EseLocalStore::LOCAL_STORE_FLAG_USE_LAST_MODIFIED_ON_PRIMARY_COLUMN);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txn;
            CODING_ERROR_ASSERT(store->CreateTransaction(txn).IsSuccess());

            ILocalStore::EnumerationSPtr en;
            CODING_ERROR_ASSERT(store->CreateEnumerationByTypeAndKey(txn, type, keyPrefix, en).IsSuccess());

            Trace.WriteInfo(testTraceType, "Verifying lastModifiedOnPrimary for all items.");

            while (en->MoveNext().IsSuccess())
            {
                wstring key;
                FILETIME filetime;

                en->CurrentKey(key);
                en->CurrentLastModifiedOnPrimaryFILETIME(filetime);

                _int64 filetimeInt64 = static_cast<_int64>(DateTime(filetime).AsLargeInteger.QuadPart);

                Trace.WriteInfo(
                    testTraceType,
                    "Key: ({0}) File-time: (Expected: {1}, Actual: {2})",
                    key,
                    keyFileTimeMap.at(key),
                    filetimeInt64);

                CODING_ERROR_ASSERT(keyFileTimeMap.at(key) == filetimeInt64);

                keyFileTimeMap.erase(key);
            }

            CODING_ERROR_ASSERT(keyFileTimeMap.empty());
            CODING_ERROR_ASSERT(txn->Commit().IsSuccess());

            Trace.WriteInfo(testTraceType, "Closing the database.");
        }

        StoreConfig::GetConfig().Test_Reset();
    }

    /// <summary>
    /// Create a database, add some entries, then do a full back up the database.
    /// Then restore the backed up database and verify if the records match.
    /// </summary>
    BOOST_AUTO_TEST_CASE(FullBackupRestore)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-FullBackupRestore");
        std::wstring backupDir2 = baseDir.Path + L"\\Backup2";

        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;
        IndexRange range(1000);

        Trace.WriteInfo(TraceType, "Database path:{0}", baseDir.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(baseDir);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Backing up existing database.");

            ErrorCode backupResult = store->Backup(backupDir2);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());
        }

        {
            Trace.WriteInfo(TraceType, "Restoring backed up database to validate contents with original database.");
            auto backedupStore = make_shared<EseLocalStore>(backupDir2, L"FabricLocalstore.edb", 0L);

            ErrorCode result1 = backedupStore->PrepareRestoreForValidation(
                backupDir2,
                TestStoreInstanceName);
            CODING_ERROR_ASSERT(result1.IsSuccess());

            const TableValidator backupValidator(*backedupStore);

            Trace.WriteInfo(TraceType, "Validating restored database.");
            backupValidator.BulkValidate(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Closing the restored database after validation.");
        }
    }

    /// <summary>
    /// Create a database, add some entries, then back up the database.
    /// Then add more entries and do an incremental backup.
    /// Then restore the backed up database and verify if the records match.
    /// </summary>
    BOOST_AUTO_TEST_CASE(IncrementalBackupRestore)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-IncrementalBackupRestore");
        std::wstring backupDirFull = baseDir.Path + L"\\BackupFull";
        std::wstring backupDirIncremental = baseDir.Path + L"\\BackupIncremental";
        std::wstring backupDirMerged = baseDir.Path + L"\\BackupMerged";

        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;

        // Generate 4 records, in the first full backup, insert the first 2 records. In incremental 
        // backup, insert the remaining 2. Then restore and compare with all 4 records.
        IndexRange range1(0, 1, 4);
        IndexRange range2(0, 1, 2);
        IndexRange range3(2, 1, 4);

        Trace.WriteInfo(TraceType, "Database path:{0}", baseDir.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(baseDir, 0L, true);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Backing up existing database.");

            ErrorCode backupResult = store->Backup(backupDirFull);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());

            Trace.WriteInfo(TraceType, "Populating database with additional generated data.");
            validator.BulkInsert(identityGenerator, range3);

            Trace.WriteInfo(TraceType, "Backing up database incrementally.");
            backupResult = store->Backup(backupDirIncremental, StoreBackupOption::Incremental);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());
        }

        {
            RestoreHelper(baseDir, range1);
        }
    }

    /// <summary>
    /// Tests whether truncate option can be used successfully with or without incremental backup.
    /// Create a database without incremental backup enabled. Add entries and truncate
    /// the backup. It should succeed. Then reopen the database with incremental backup
    /// enabled this time, add more entries and truncate the backup. This call also should
    /// succeed.
    /// </summary>
    BOOST_AUTO_TEST_CASE(Truncate)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-Truncate");
        
        IndexRange range1(0, 1, 1000);
        IndexRange range2(1000, 1, 2000);
        IndexRange range3(0, 1, 2000);

        TruncateHelper(baseDir, false, range1, range1);
        TruncateHelper(baseDir, true, range2, range3);
    }

    /// <summary>
    /// Tests whether truncate option can be used successfully when multiple threads
    /// are trying to truncate logs.
    /// There exists a known concurrency bug in log truncation in ESENT.dll. For details, please
    /// see comments related to truncation in eseinstance.cpp, Backup() method.
    /// This unit test tests the WORKAROUND we have applied.
    /// </summary>
    BOOST_AUTO_TEST_CASE(Truncate2)
    {        
        ManualResetEvent waiter1;
        ManualResetEvent waiter2;

        AsyncOperationSPtr operation1 = AsyncOperation::CreateAndStart<BackupTruncate1AsyncOperation>(
            L"1",
            L"FabricTest-LocalStore-Truncate2-1",
            [&](Common::AsyncOperationSPtr const &)
            {
                waiter1.Set();
            },
            nullptr);
        
        AsyncOperationSPtr operation2 = AsyncOperation::CreateAndStart<BackupTruncate1AsyncOperation>(
            L"2",
            L"FabricTest-LocalStore-Truncate2-2",
            [&](Common::AsyncOperationSPtr const &)
            {
                waiter2.Set();
            },
            nullptr);
        
        waiter1.WaitOne();
        waiter2.WaitOne();

        auto * result1 = AsyncOperation::End<BackupTruncate1AsyncOperation>(operation1);
        auto * result2 = AsyncOperation::End<BackupTruncate1AsyncOperation>(operation2);

        CODING_ERROR_ASSERT(result1->Error.IsSuccess());
        CODING_ERROR_ASSERT(result2->Error.IsSuccess());
    }
    
    /// <summary>
    /// Create a database, add some entries, then back up the database.
    /// Then repeatedly add more entries and do multiple incremental backups.
    /// Then restore the backed up database and verify if the records match.
    /// </summary>
    BOOST_AUTO_TEST_CASE(IncrementalBackupRestore2)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-IncrementalBackupRestore2");
        std::wstring backupDirFull = baseDir.Path + L"\\BackupFull";
        std::wstring backupDirIncremental = baseDir.Path + L"\\BackupIncremental";
        std::wstring backupDirMerged = baseDir.Path + L"\\BackupMerged";

        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;

        IndexRange range1(0, 1, 100);
        IndexRange range2(0, 1, 50);

        const int StepSize = 10;

        int dirCounter = 0;

        Trace.WriteInfo(TraceType, "Database path:{0}", baseDir.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(baseDir, 0L, true);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Backing up existing database.");

            ErrorCode backupResult = store->Backup(backupDirFull);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());

            Trace.WriteInfo(TraceType, "Populating database with additional generated data.");

            for (int recordCount = range2.Limit; recordCount < range1.Limit; recordCount += StepSize, dirCounter++)
            {
                IndexRange range(recordCount, 1, recordCount + StepSize);
                validator.BulkInsert(identityGenerator, range);

                std::wstring dir = wformatString("{0}{1}", backupDirIncremental, dirCounter);
                backupResult = store->Backup(dir, StoreBackupOption::Incremental);
                CODING_ERROR_ASSERT(backupResult.IsSuccess());
            }

            Trace.WriteInfo(TraceType, "Completed multiple incremental backups.");
        }

        {
            // now we need to merge the contents of the 2 directories (BackupFull and BackupIncremental), do a restore and verify
            // if the contents match whatever was inserted.

            ErrorCode mergeResult = Directory::Create2(backupDirMerged);
            CODING_ERROR_ASSERT(mergeResult.IsSuccess());

            // ESE puts the full backup contents into a sub folder named 'new' under the folder we specify.
            mergeResult = Directory::Copy(backupDirFull + L"\\new", backupDirMerged, true);
            CODING_ERROR_ASSERT(mergeResult.IsSuccess());

            for (int i = 0; i < dirCounter; i++)
            {
                std::wstring dir = wformatString("{0}{1}", backupDirIncremental, i);
                mergeResult = Directory::Copy(dir, backupDirMerged, true);
                CODING_ERROR_ASSERT(mergeResult.IsSuccess());
            }

            Trace.WriteInfo(TraceType, "Restoring database from merged contents of full and multiple incremental backups.");
            auto backedupStore = make_shared<EseLocalStore>(backupDirMerged, L"FabricLocalstore.edb", 0L);
			
			ErrorCode result1 = backedupStore->PrepareRestoreForValidation(
                backupDirMerged,
                TestStoreInstanceName);
            CODING_ERROR_ASSERT(result1.IsSuccess());

            const TableValidator validator(*backedupStore);

            Trace.WriteInfo(TraceType, "Validating restored database.");
            validator.BulkValidate(identityGenerator, range1);

            Trace.WriteInfo(TraceType, "Closing the restored database after validation.");
        }
    }

    BOOST_AUTO_TEST_CASE(UpgradeToIncrementalBackup)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-UpgradeToIncrementalBackup");
        UpgradeToIncrementalBackupHelper(baseDir);
    }

    BOOST_AUTO_TEST_CASE(DowngradeToFullBackup)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-DowngradeToFullBackup");
        UpgradeToIncrementalBackupHelper(baseDir);

        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;

        // From incremental backup helper method that creates 4 records, 
        // downgrade the database to accept only full backups, then add an additional 2 records
        // then validate existence of all 6 records
        IndexRange range1(0, 1, 6);
        IndexRange range2(4, 1, 6);

        {
            Trace.WriteInfo(TraceType, "Reopening database with incremental backup disabled again.");
            auto store = CreateTestLocalStore(baseDir, 0L);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range1);
        }
    }

    BOOST_AUTO_TEST_CASE(BackupRestoreAfterIncrementalUpgrade)
    {
        TemporaryCurrentDirectory baseDir(L"FabricTest-LocalStore-BackupRestoreAfterIncrementalUpgrade");

        UpgradeToIncrementalBackupHelper(baseDir);

        std::wstring backupDirFull = baseDir.Path + L"\\BackupFull";
        std::wstring backupDirIncremental = baseDir.Path + L"\\BackupIncremental";
        std::wstring backupDirMerged = baseDir.Path + L"\\BackupMerged";

        IdentityGenerator identityGenerator;
        Plus5Generator plus5Generator;

        // From incremental backup helper method that creates 4 records, 
        // downgrade the database to accept only full backups, then add an additional 2 records
        // then validate existence of all 6 records
        IndexRange range1(0, 1, 6);
        IndexRange range2(4, 1, 6);

        {
            Trace.WriteInfo(TraceType, "Opening database.");
            auto store = CreateTestLocalStore(baseDir, 0L, true);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            Trace.WriteInfo(TraceType, "Backing up existing database.");

            ErrorCode backupResult = store->Backup(backupDirFull);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with additional generated data.");
            validator.BulkInsert(identityGenerator, range2);

            backupResult = store->Backup(backupDirIncremental, StoreBackupOption::Enum::Incremental);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());
        }

        {
            RestoreHelper(baseDir, range1);
        }
    }

    
    class StoreTerminationDelayer
    {
        DENY_COPY(StoreTerminationDelayer);
    public:
        StoreTerminationDelayer(ILocalStore & store) : store_(store), delayIntroducedEvent_(), delayCompletionTime_()
        { }

        __declspec(property(get = get_DelayCompletionTime)) Common::DateTime DelayCompletionTime;

        Common::DateTime get_DelayCompletionTime() const { return delayCompletionTime_; }

        void AddDelay(Common::TimeSpan delay)
        {
            AddDelayAsynchronously(delay);

            delayIntroducedEvent_.WaitOne();
        }

    private:
        void AddDelayAsynchronously(Common::TimeSpan delay)
        {
            Threadpool::Post([this, delay]() { this->DelayTermination(delay); });
        }

        void DelayTermination(Common::TimeSpan delay)
        {
            // Create a transaction and hold on to it till the specified delay
            // This cause store termination to be delayed until destruction of the transaction
            ILocalStore::TransactionSPtr txSPtr;
            CODING_ERROR_ASSERT(store_.CreateTransaction(txSPtr).IsSuccess());
            delayIntroducedEvent_.Set();
            Sleep(static_cast<DWORD>(delay.TotalMilliseconds()));
            delayCompletionTime_ = Common::DateTime::Now();
        }

        ILocalStore & store_;
        Common::AutoResetEvent delayIntroducedEvent_;
        Common::DateTime delayCompletionTime_;
    };

    BOOST_AUTO_TEST_CASE(Terminate)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-Terminate"));
        const IdentityGenerator identityGenerator;

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            unique_ptr<EseLocalStore> store = make_unique<EseLocalStore>(
                directory.Path,
                L"FabricLocalstore.edb",
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());
            StoreTerminationDelayer storeTerminationDelayer(*store);

            storeTerminationDelayer.AddDelay(Common::TimeSpan::FromSeconds(1));

            store->Terminate();
            store.reset();

            CODING_ERROR_ASSERT(Common::DateTime::Now() >= storeTerminationDelayer.DelayCompletionTime && storeTerminationDelayer.DelayCompletionTime.Ticks > 0);
        }
    }

    BOOST_AUTO_TEST_CASE(VersionStoreTermination)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-VersionStoreTermination"));
        const IdentityGenerator identityGenerator;

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        {
            EseLocalStoreSettings settings(L"FabricLocalstore.edb", L"");
            settings.MaxVerPages = 10;
            auto store = make_unique<EseLocalStore>(
                directory.Path,
                settings,
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            VERIFY_IS_TRUE(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txSPtr;
            VERIFY_IS_TRUE(store->CreateTransaction(txSPtr).IsSuccess());

            int keyCount = 1000;
            vector<byte> data;
            data.resize(1);

            for (auto ix=0; ix<keyCount; ++ix)
            {
                auto error = store->Insert(
                    txSPtr,
                    L"TestType",
                    wformatString("TestKey_{0}", ix),
                    data.data(),
                    data.size());
                VERIFY_IS_TRUE(error.IsSuccess());
            }

            auto error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());

            bool hitExpectedError = false;

            for (auto ix=0; ix<8*1024; ++ix)
            {
                data.push_back(static_cast<byte>(ix));
            }

            VERIFY_IS_TRUE(store->CreateTransaction(txSPtr).IsSuccess());

            for (auto ix=0; ix<keyCount; ++ix)
            {
                error = store->Update(
                    txSPtr,
                    L"TestType",
                    wformatString("TestKey_{0}", ix),
                    0,
                    wformatString("TestKey_{0}", ix),
                    data.data(),
                    data.size());
                VERIFY_IS_TRUE(error.IsSuccess() || error.IsError(ErrorCodeValue::StoreTransactionTooLarge));

                if (!hitExpectedError && error.IsError(ErrorCodeValue::StoreTransactionTooLarge))
                {
                    hitExpectedError = true;
                }
            }

            VERIFY_IS_TRUE(hitExpectedError);

            // Termination should succeed after encountering JET_errVersionStoreOutOfMemory
            //
            error = store->Terminate();

            VERIFY_IS_TRUE(error.IsSuccess());
        }

        // Re-open should succeed against the same store
        //
        {
            EseLocalStoreSettings settings(L"FabricLocalstore.edb", L"");
            auto store = make_unique<EseLocalStore>(
                directory.Path,
                settings,
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            VERIFY_IS_TRUE(store->Initialize(TestStoreInstanceName).IsSuccess());

            ILocalStore::TransactionSPtr txSPtr;
            VERIFY_IS_TRUE(store->CreateTransaction(txSPtr).IsSuccess());

            vector<byte> data;
            data.resize(100);

            auto error = store->Insert(
                txSPtr,
                L"TestType2",
                L"TestKey",
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());

            error = store->Terminate();
            VERIFY_IS_TRUE(error.IsSuccess());
        }
    }

    BOOST_AUTO_TEST_CASE(DefragmentationLifetime)
    {
        const TemporaryCurrentDirectory directory(std::wstring(L"FabricTest-LocalStore-DefragmentationLifetime"));
        const IdentityGenerator identityGenerator;

        Trace.WriteInfo(TraceType, "Database path:{0}", directory.Path.c_str());

        StoreConfig::GetConfig().MaxDefragFrequencyInMinutes = 1;
        StoreConfig::GetConfig().DefragThresholdInMB = 0;

        IndexRange range(10000);

        {
            unique_ptr<EseLocalStore> store = make_unique<EseLocalStore>(
                directory.Path,
                L"FabricLocalstore.edb",
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range);

            store.reset();
        }

        Random random;

        for (auto ix=0; ix<50; ++ix)
        {
            // Re-open store to trigger defragmentation callback while closing and releasing
            // store in parallel
            //
            unique_ptr<EseLocalStore> store = make_unique<EseLocalStore>(
                directory.Path,
                L"FabricLocalstore.edb",
                EseLocalStore::LOCAL_STORE_FLAG_NONE);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            Sleep(random.Next(0, 100));

            store.reset();
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    shared_ptr<EseLocalStore> TestEseLocalStore::CreateTestLocalStore(TemporaryCurrentDirectory const & directory, LONG flags, bool enableIncrementalBackup)
    {
        EseLocalStoreSettings settings(L"FabricLocalstore.edb", L"");
        settings.IsIncrementalBackupEnabled = enableIncrementalBackup;
        return make_shared<EseLocalStore>(directory.Path, settings, flags);
    }

    void TestEseLocalStore::TruncateHelper(
        TemporaryCurrentDirectory const & baseDir,
        bool enableIncrementalBackup,
        IndexRange const & creationRange,
        IndexRange const & verificationRange)
    {
        std::wstring backupDir = L"";

        IdentityGenerator identityGenerator;
        IndexRange range(1000);

        Trace.WriteInfo(TraceType, "Database path:{0}", baseDir.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database.");
            auto store = CreateTestLocalStore(baseDir, enableIncrementalBackup);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, creationRange);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, verificationRange);

            Trace.WriteInfo(TraceType, "Backing up existing database.");

            ErrorCode backupResult = store->Backup(backupDir);
            CODING_ERROR_ASSERT(backupResult.IsSuccess());
        }
    }

    /// <summary>
    /// Helper method that restores a database from a full backup and an incremental backup directory.
    /// </summary>
    /// <param name="baseDir">The base dir under which the current database, the full backup folder and the incremental backup folder exists.</param>
    /// <param name="range">The range to verify after restoration.</param>
    void TestEseLocalStore::RestoreHelper(TemporaryCurrentDirectory const & baseDir, IndexRange const & range)
    {
        std::wstring backupDirFull = baseDir.Path + L"\\BackupFull";
        std::wstring backupDirIncremental = baseDir.Path + L"\\BackupIncremental";
        std::wstring backupDirMerged = baseDir.Path + L"\\BackupMerged";

        // now we need to merge the contents of the 2 directories (BackupFull and BackupIncremental), do a restore and verify
        // if the contents match whatever was inserted.

        ErrorCode mergeResult = Directory::Create2(backupDirMerged);
        CODING_ERROR_ASSERT(mergeResult.IsSuccess());

        // ESE puts the full backup contents into a sub folder named 'new' under the folder we specify.
        mergeResult = Directory::Copy(backupDirFull + L"\\new", backupDirMerged, true);
        CODING_ERROR_ASSERT(mergeResult.IsSuccess());

        mergeResult = Directory::Copy(backupDirIncremental, backupDirMerged, true);
        CODING_ERROR_ASSERT(mergeResult.IsSuccess());

        Trace.WriteInfo(TraceType, "Restoring database from merged contents of full and incremental backups.");
        auto backedupStore = make_shared<EseLocalStore>(backupDirMerged, L"FabricLocalstore.edb", 0L);

        ErrorCode result1 = backedupStore->PrepareRestoreForValidation(
            backupDirMerged,
            TestStoreInstanceName);
        CODING_ERROR_ASSERT(result1.IsSuccess());

        const TableValidator validator(*backedupStore);

        IdentityGenerator identityGenerator;

        Trace.WriteInfo(TraceType, "Validating restored database.");
        validator.BulkValidate(identityGenerator, range);

        Trace.WriteInfo(TraceType, "Closing the restored database after validation.");
    }

    void TestEseLocalStore::UpgradeToIncrementalBackupHelper(TemporaryCurrentDirectory const & baseDir)
    {
        IdentityGenerator identityGenerator;

        // Insert 2 records with incremental backup disabled. then insert 2 more by reopening the database
        // with incremental enabled. 
        // Then validate existence of all 4 records
        IndexRange range1(0, 1, 4);
        IndexRange range2(0, 1, 2);
        IndexRange range3(2, 1, 4);

        Trace.WriteInfo(TraceType, "Database path:{0}", baseDir.Path.c_str());

        {
            Trace.WriteInfo(TraceType, "Creating database with incremental backup disabled.");
            auto store = CreateTestLocalStore(baseDir, 0L);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range2);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range2);
        }

        {
            Trace.WriteInfo(TraceType, "Reopening database with incremental backup enabled.");
            auto store = CreateTestLocalStore(baseDir, 0L, true);
            CODING_ERROR_ASSERT(store->Initialize(TestStoreInstanceName).IsSuccess());

            const TableValidator validator(*store);

            Trace.WriteInfo(TraceType, "Populating database with generated data.");
            validator.BulkInsert(identityGenerator, range3);

            Trace.WriteInfo(TraceType, "Validating contents against generated data.");
            validator.BulkValidate(identityGenerator, range1);
        }
    }

    bool TestEseLocalStore::TestcaseSetup()
    {
        StoreConfig::GetConfig().Test_Reset();
        return true;
    }
}
