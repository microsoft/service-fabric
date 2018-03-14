// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //* A simple extension to KHashTable for the Native data stack runtime - as a default
        //  adhoc HashTable based on doubling and halfing of hash buckets rounded to the nearest
        //  prime number up to approx 3G buckets
        //
        template <class KeyType, class DataType>
        class KAutoHashTable : public KHashTable<KeyType, DataType>
        {
            K_DENY_COPY(KAutoHashTable);

        private:
            __forceinline void ResizeUpIf()
            {
                ULONG savedIx = _currentPrimeIx;

                while ((this->Saturation(_gDoublePrimes[_currentPrimeIx]) > _autoUpsizeThreashold) &&
                    (_currentPrimeIx < (_gDoublePrimesSize - 1)))               // Increase if above 60%
                {
                    // Resize up:
                    _currentPrimeIx++;
                }

                if (savedIx != _currentPrimeIx)
                {
                    if (!NT_SUCCESS(__super::Resize(_gDoublePrimes[_currentPrimeIx])))
                    {
                        _currentPrimeIx = savedIx;
                    }
                }
            }

            __forceinline void ResizeDownIf()
            {
                ULONG savedIx = _currentPrimeIx;

                while ((this->Saturation(_gDoublePrimes[_currentPrimeIx]) < _autoDownsizeThreashold) &&
                    (_currentPrimeIx > 0))                                       // Decrease if below 30%
                {
                    // Resize down using the prev in sequence
                    _currentPrimeIx--;
                }

                if (savedIx != _currentPrimeIx)
                {
                    if (!NT_SUCCESS(__super::Resize(_gDoublePrimes[_currentPrimeIx])))
                    {
                        _currentPrimeIx = savedIx;
                    }
                }
            }

        public:
            static const ULONG          DefaultAutoUpsizeThresholdPrectage = 60;
            static const ULONG          DefaultAutoDownsizeThresholdPrectage = 30;

            FAILABLE KAutoHashTable(
                __in ULONG Size,
                __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
                __in KAllocator& Alloc)
                : KHashTable<KeyType, DataType>(Alloc)
            {
                if (NT_SUCCESS(this->Status()))
                {
                    _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
                    _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
                    _autoSizeIsEnabled = true;
                    _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
                    Size = _gDoublePrimes[_currentPrimeIx];
                    this->SetConstructorStatus(__super::Initialize(Size, Func));
                }
            }

            FAILABLE KAutoHashTable(
                __in ULONG Size,
                __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
                __in typename KHashTable<KeyType, DataType>::EqualityComparisonFunctionType EqualityComparisonFunctionType,
                __in KAllocator& Alloc)
                : KHashTable<KeyType, DataType>(Alloc)
            {
                if (NT_SUCCESS(this->Status()))
                {
                    _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
                    _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
                    _autoSizeIsEnabled = true;
                    _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
                    Size = _gDoublePrimes[_currentPrimeIx];
                    this->SetConstructorStatus(__super::Initialize(Size, Func, EqualityComparisonFunctionType));
                }
            }

            // Default constructor
            //
            // This allows the hash table to be embedded in another object and be initialized during later execution once
            // the values for Initialize() are known.
            //
            NOFAIL KAutoHashTable(__in KAllocator& Alloc)
                : KHashTable<KeyType, DataType>(Alloc)
            {
                _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
                _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
                _autoSizeIsEnabled = true;
                _currentPrimeIx = 0;
            }

            // Move constructor.
            NOFAIL KAutoHashTable(__in KAutoHashTable&& Src)
                : KHashTable<KeyType, DataType>(Ktl::Move(*((KHashTable<KeyType, DataType>*)&Src)))
            {
                _autoUpsizeThreashold = Src._autoUpsizeThreashold;
                _autoDownsizeThreashold = Src._autoDownsizeThreashold;
                _autoSizeIsEnabled = Src._autoSizeIsEnabled;
                _currentPrimeIx = Src._currentPrimeIx;
                Src._currentPrimeIx = 0;
                Src._autoSizeIsEnabled = true;
                Src._autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
                Src._autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
            }

            // Move assignment operator.
            NOFAIL KAutoHashTable& operator=(__in KAutoHashTable&& Src)
            {
                if (&Src == this)
                {
                    return *this;
                }

                __super::operator=(Ktl::Move(Src));
                _autoUpsizeThreashold = Src._autoUpsizeThreashold;
                _autoDownsizeThreashold = Src._autoDownsizeThreashold;
                _autoSizeIsEnabled = Src._autoSizeIsEnabled;
                _currentPrimeIx = Src._currentPrimeIx;
                Src._currentPrimeIx = 0;
                Src._autoSizeIsEnabled = true;
                Src._autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
                Src._autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
                return *this;
            }

            //* Post CTOR Initializers
            NTSTATUS Initialize(__in ULONG Size, __in typename KHashTable<KeyType, DataType>::HashFunctionType Func)
            {
                _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
                Size = _gDoublePrimes[_currentPrimeIx];
                return __super::Initialize(Size, Func);
            }

            NTSTATUS Initialize(
                __in ULONG Size,
                __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
                __in typename KHashTable<KeyType, DataType>::EqualityComparisonFunctionType EqualityComparitionFunc)
            {
                _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
                Size = _gDoublePrimes[_currentPrimeIx];
                return __super::Initialize(Size, Func, EqualityComparitionFunc);
            }

            //* Disable or Enable automatic resizing - when enabled resizing will be attempted
            void SetAutoSizeEnable(__in bool ToEnabled)
            {
                if (ToEnabled && !_autoSizeIsEnabled)
                {
                    ResizeUpIf();
                    ResizeDownIf();
                }

                _autoSizeIsEnabled = ToEnabled;
            }

            //* Accessors for Up/Downsize thresholds
            ULONG GetUpsizeThreshold() { return _autoUpsizeThreashold; }
            ULONG GetDownsizeThreshold() { return _autoDownsizeThreashold; }
            void  SetUpsizeThreshold(ULONG NewValue) { _autoUpsizeThreashold = newValue; }
            void  SetDownsizeThreshold(ULONG NewValue) { _autoDownsizeThreashold = newValue; }

            // Put
            //
            // Adds or updates the item.
            //
            // Parameters:
            //      Key           The key under which to store the data item
            //      Data          The data item to store.
            //      ForceUpdate   If TRUE, always write the item, even if it is an update.
            //                    If FALSE, only create, but don't update.
            //      PreviousValue Retrieves the previously existing value if one exists.
            //                    Typically used in combination with ForceUpdate==FALSE.
            //
            // Return values:
            // Return values:
            //      STATUS_SUCCESS - Item was inserted and there was no previous item with
            //          the same key.
            //      STATUS_OBJECT_NAME_COLLISION - Item was not inserted because there was an
            //          existing item with the same key and ForceUpdate was false.
            //      STATUS_OBJECT_NAME_EXISTS - Item replaced a previous item.
            //      STATUS_INSUFFICIENT_RESOURCES - Item was not inserted.
            //
            NTSTATUS Put(
                __in  const KeyType&  Key,
                __in  const DataType& Data,
                __in  BOOLEAN   ForceUpdate = TRUE,
                __out_opt DataType* PreviousValue = nullptr)
            {
                if (_autoSizeIsEnabled)
                {
                    ResizeUpIf();
                }
                return __super::Put(Key, Data, ForceUpdate, PreviousValue);
            }

            // Remove
            //
            // Removes the specified Key and its associated data item from the table
            //
            // Return values:
            //      STATUS_SUCCESS
            //      STATUS_NOT_FOUND
            //
            NTSTATUS Remove(__in const KeyType& Key, __out  DataType* Item = nullptr)
            {
                NTSTATUS result = __super::Remove(Key, Item);
                if (NT_SUCCESS(result))
                {
                    if (_autoSizeIsEnabled)
                    {
                        ResizeDownIf();
                    }
                }

                return result;
            }

            // Resize
            //
            // Resizes the hash table.
            // Parameters:
            //      NewSize         This typically should be 50% larger than the previous size,
            //                      but still a prime number.
            //
            //
            NTSTATUS Resize(__in ULONG NewSize)
            {
                _currentPrimeIx = ClosestNextPrimeNumberIx(NewSize);
                NewSize = _gDoublePrimes[_currentPrimeIx];
                return __super::Resize(NewSize);
            }

            // Clear
            //
            // Remove all entries from the table.
            //
            VOID Clear()
            {
                __super::Clear();
                _currentPrimeIx = 0;
            }

            __forceinline ULONG TestGetCurrentIx()
            {
                return _currentPrimeIx;
            }

        private:
            static ULONG ClosestNextPrimeNumberIx(__in ULONG From)
            {
                ULONG   ix = 0;

                while ((ix < _gDoublePrimesSize) && (_gDoublePrimes[ix] < From)) ix++;
                return ix;
            }

        private:
            ULONG       _currentPrimeIx;
            ULONG       _autoDownsizeThreashold;
            ULONG       _autoUpsizeThreashold;
            bool        _autoSizeIsEnabled;

        public:
            static ULONG const      _gDoublePrimes[];
            static const ULONG      _gDoublePrimesSize;
        };

        //* Table of prime numbers at approx 2* intervals out to the limits of a ULONG.
        //  These values are intented to used to data-drive KAutoHashTable auto upsizing 
        //  and downsizing
        //
        template <class KeyType, class DataType>
        ULONG const KAutoHashTable<KeyType, DataType>::_gDoublePrimes[] =
        {
            1,          3,          7,          17,         37,         79,         163,        331,        673,        
            1361,       2729,       5471,       10949,      21911,      43853,      87719,      175447,     350899,     
            701819,     1403641,    2807303,    5614657,    11229331,   22458671,   44917381,   89834777,   179669557,  
            359339171,  718678369,  1437356741, 2874713497
        };

        template <class KeyType, class DataType>
        const ULONG KAutoHashTable<KeyType, DataType>::_gDoublePrimesSize = sizeof(_gDoublePrimes) / sizeof(ULONG);
    }
}
