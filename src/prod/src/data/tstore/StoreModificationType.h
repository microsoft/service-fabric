// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      namespace StoreModificationType
      {
         enum Enum
         {
            Add = 0,
            Remove = 1,
            Update = 2,
            PartialUpdate = 3,
            Get = 4,
            Clear = 5,
            Checkpoint = 6,
            Copy = 7,
            Pause = 8
         };
      }

      namespace LockingHints
      {
         enum Enum
         {
            None = 0,
            NoLock = 1,
            ReadPast = 2,
            UpdateLock = 4,
         };
      }
   }
}
