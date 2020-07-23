// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

SortedItemComparer::SortedItemComparer()
{
}

SortedItemComparer::~SortedItemComparer()
{
}

NTSTATUS IntComparer::Create(
   __in KAllocator & allocator,
   __out IntComparer::SPtr & result)
{
   result = _new(INT_COMPARER_TAG, allocator) IntComparer();

   if (!result)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   return STATUS_SUCCESS;
}

int IntComparer::Compare(__in const int& x, __in const int& y) const
{
   if (x < y)
   {
      return -1;
   }
   else if (x > y)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}
