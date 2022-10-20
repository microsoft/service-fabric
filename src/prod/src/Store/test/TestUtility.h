// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using namespace std;
using namespace Common;

namespace Store
{
   class ITableGenerator
    {
    public:
        virtual void GetType(/*out*/wstring & type) const = 0;
        virtual void GetKey(int index, /*out*/wstring & key) const = 0;
        virtual void GetValue(int index, /*out*/vector<byte> & value) const = 0;
    };

    struct StoreItem
    {
        wstring Type;
        wstring Key;
        vector<byte> Value;
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

                this->DumpItem(L"Validate ", expected);

                bool found = false;
                do
                {
                    enumeration->CurrentType(actual.Type);
                    enumeration->CurrentKey(actual.Key);
                    enumeration->CurrentValue(actual.Value);

                    if (StringUtility::Compare(actual.Type, expected.Type) == 0 &&
                        StringUtility::Compare(actual.Key, expected.Key) == 0 &&
                        actual.Value == expected.Value)
                    {
                        found = true;
                        break;
                    }
                }while(enumeration->MoveNext().IsSuccess());
                CODING_ERROR_ASSERT(found);
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

                CODING_ERROR_ASSERT(StringUtility::Compare(actual.Type, expected.Type)==0);
                CODING_ERROR_ASSERT(StringUtility::Compare(actual.Key, expected.Key)==0);
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
        virtual void GetValue(int index, /*out*/vector<byte> & value) const
        {
            value.clear();
            auto p = reinterpret_cast<byte *>(&index);
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
        virtual void GetValue(int index, /*out*/vector<byte> & value) const
        {
            value.clear();
            auto p = reinterpret_cast<byte *>(&index);
            for (int i = 0; i<sizeof(index)+5; i++) { value.push_back(p[i%sizeof(index)]); }
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
        virtual void GetValue(int index, /*out*/vector<byte> & value) const
        {
            value.clear();
            value.resize(static_cast<size_t>(valueSize_));

            auto p = const_cast<byte *>(value.data());
            for (size_t i = 0; i<valueSize_; i++)
            {
                p[i] = static_cast<byte>(((index + i) ^ (i >> 8) ^ (i >> 16)) & 0xff);
            }
        }
    private:
        const wstring type_;
        const size_t keySize_;
        const size_t valueSize_;
    };
    
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
}
