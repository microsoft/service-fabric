// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        template<typename T>
        class Sort
        {
        public:
            typedef KDelegate<LONG32(__in T const & KeyOne, __in T const & KeyTwo)> ComparisonFunctionType;

        public:
            static LONG32 BinarySearch(
                __in T const & value,
                __in bool isAscending,
                __in ComparisonFunctionType comparer,
                __inout KSharedPtr<KSharedArray<T>> & array)
            {
                return BinarySearch(value, isAscending, comparer, *array);
            }

            // BinarySearch Used for CheckpointManager Binary Insertion Sort
            // BinarySearch for the right position to insert the Metadata into a descending order Metadata array, which runs O(log n) in average
            // Use iteration instead of recursion which increases stack for every recursive call.
            //
            // For the CheckpointManager Binary Insertion Sort usage, the function takes in compared StateProvider name and descending sorted Metadata array, 
            // return the complement of the right position to insert the input name. In the functions, every round picks the middle point to compare 
            // with the input name, because of we need the array in descending order, so if the input name is larger than the name in middle point, 
            // then we choose left part. Otherwise, choose right part, until right cursor less then or equal to left cursor which means we find the 
            // right position to insert.
            //
            // For general usage, the function returns the index of the given value in the array. But if the array does not have the input value, it returns
            // the complement of the position which is the index of the first element that is larger than the value(in ascending case). So if the function 
            // returns a negative value, use a complement to get the position to insert to keep order.
            static LONG32 BinarySearch(
                __in T const & value,
                __in bool isAscending,
                __in ComparisonFunctionType comparer,
                __in KArray<T> const & array)
            {
                // if the ordering of the array is different, just change the sign of the comparer
                // Get the initial range of the array
                LONG32 left = 0, right = array.Count() - 1, sign = isAscending ? -1 : 1;

                // Every time pick the middle point to compare with the input value. If the array is in 
                // descending order, and the input value is larger than the value in middle point, then we choose left part,
                // otherwise, choose right part, until right cursor less then or equal to left cursor which means we find the 
                // right position to insert.
                while (left <= right)
                {
                    // instead of using (left + right) /2 to avoid right is very large and the add operation overflow
                    LONG32 mid = left + (right - left) / 2;

                    // If the value is found in the array, return the position
                    if (comparer(value, (array)[mid]) == 0)
                    {
                        return mid;
                    }

                    // If the input value is less than the middle point value 
                    if (sign * comparer(value, (array)[mid]) < 0)
                    {
                        left = mid + 1;
                    }
                    else
                    {
                        // The situation that input name is larger than the middle point name 
                        right = mid - 1;
                    }
                }

                // The array does not contain the input value, return the complement of the position to insert, which is a negative value
                return ~left;
            }

            // QuickSort used in statemanager.cpp to sort the recoveredMetadataList in place and in descending order
            // QuickSort is a comparison sort, on average, the algorithm takes O(n long n) comparisons to sort n items
            // the worst case takes O(n ^ 2). 
            // 
            // The algorithm is first picking a pivot from the array, then do the partitioning, which reorder the array so that 
            // the items bigger then the pivot go to the left(descending order) and the items less then the pivot go to the right side
            // of pivot. Keep applying selection and partitioning until the array is sorted.
            //
            // Normally the left element is chosen as pivot, but it turns to the worst case if the array is already sorted, so here
            // we chose the middle element as pivot to avoid the common case. 
            // Using i + (j - i) / 2 instead of (i + j) / 2 to avoid the case that array size is very large and the add operation cause
            // overflow.
            //
            // In the case of array with few unique items, quick sort is not considered better then other sorting algorithm, because 
            // it easily goes to worst case.
            static void QuickSort(
                __in LONG32 left, 
                __in LONG32 right,
                __in bool isAscending,
                __in ComparisonFunctionType comparer, 
                __inout KSharedPtr<KSharedArray<T>> & array)
            {
                // if the ordering of the array is different, just change the sign of the comparer
                LONG32 sign = isAscending ? -1 : 1;

                // If the array less then 2 items, it means array is empty or 1 item, don't need to sort
                if (array->Count() < 2 && left < right)
                {
                    return;
                }

                // Check the array count first to avoid count = 0 and the minus 1 operation overflow
                // left or right cursor can not be out of metadataArray's range
                ASSERT_IFNOT(
                    left >= 0 && static_cast<ULONG>(left) <= array->Count() - 1,
                    "Invalid counts during quick sort, left: {0}, array count: {1}", 
                    left, (array->Count() - 1));
                ASSERT_IFNOT(
                    right >= 0 && static_cast<ULONG>(right) <= array->Count() - 1,
                    "Invalid counts during quick sort, right: {0}, array count: {1}", 
                    right, (array->Count() - 1));

                LONG32 i = left, j = right;
                auto pivot = (*array)[i + (j - i) / 2];

                while (i <= j)
                {
                    while (sign * comparer(pivot, (*array)[i]) < 0)
                    {
                        i++;
                    }
                    while (sign * comparer(pivot, (*array)[j]) > 0)
                    {
                        j--;
                    }

                    if (i <= j)
                    {
                        swap((*array)[i], (*array)[j]);
                        i++;
                        j--;
                    }
                }

                if (left < j)
                {
                    QuickSort(left, j, isAscending, comparer, array);
                }

                if (i < right)
                {
                    QuickSort(i, right, isAscending, comparer, array);
                }
            }

            static void QuickSort(
                __in bool isAscending,
                __in ComparisonFunctionType comparer,
                __inout KSharedPtr<KSharedArray<T>> & array)
            {
                // Check the array count here to avoid count = 0 and the minus 1 operation overflow
                if (array->Count() < 2)
                {
                    return;
                }

                QuickSort(0, static_cast<LONG32>(array->Count() - 1), isAscending, comparer, array);
            }
        };
    }
}
