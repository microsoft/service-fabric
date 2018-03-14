// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      template<typename T>
      class Sorter
      {
      public:
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
                __in IComparer<T>& comparer,
                __inout KSharedPtr<KSharedArray<T>> & array)
            {
                // if the ordering of the array is different, just change the sign of the comparer
                // Get the initial range of the array
                LONG32 left = 0, right = array->Count() - 1, sign = isAscending ? -1 : 1;

                // Every time pick the middle point to compare with the input value. If the array is in 
                // descending order, and the input value is larger than the value in middle point, then we choose left part,
                // otherwise, choose right part, until right cursor less then or equal to left cursor which means we find the 
                // right position to insert.
                while (left <= right)
                {
                    // instead of using (left + right) /2 to avoid right is very large and the add operation overflow
                    LONG32 mid = left + (right - left) / 2;

                    // If the value is found in the array, return the position
                    if(comparer.Compare(value, (*array)[mid]) == 0)
                    {
                        return mid;
                    }

                    // If the input value is less than the middle point value 
                    if (sign * comparer.Compare(value, (*array)[mid]) < 0)
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

         static void QuickSort(
            __in LONG32 left,
            __in LONG32 right,
            __in bool isAscending,
            __in IComparer<T>& comparer,
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
            KInvariant(left >= 0 && static_cast<ULONG>(left) <= array->Count() - 1);
            KInvariant(right >= 0 && static_cast<ULONG>(right) <= array->Count() - 1);

            LONG32 i = left, j = right;
            auto pivot = (*array)[i + (j - i) / 2];

            while (i <= j)
            {
               while (sign * comparer.Compare(pivot, (*array)[i]) < 0)
               {
                  i++;
               }
               while (sign * comparer.Compare(pivot, (*array)[j]) > 0)
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
            __in IComparer<T>& comparer,
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
