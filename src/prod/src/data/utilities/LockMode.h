// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOCK_MODE_TAG 'mmLI'

namespace Data
{
   namespace Utilities
   {
      namespace LockMode
      {
         enum Enum : ULONG32
         {
            Free,
            Shared,
            Exclusive,
            Update
            //IntentShared
         };
      }

      /// <summary>
      /// Represents lock compatibility values.
      /// </summary>
      namespace LockCompatibility
      {
         enum Enum : ULONG32
         {
            Conflict = 0,
            NoConflict = 1,
         };
      }
   }
}
