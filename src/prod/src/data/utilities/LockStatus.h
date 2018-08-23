// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace Utilities
   {
      namespace LockStatus
      {
         enum Enum
         {
            Invalid = 1,
            Granted = 2,
            Timeout = 3,
            Pending = 4
         };
      }

      namespace UnlockStatus
      {
         enum Enum
         {
            Invalid,
            Success,
            UnknownResource,
            NotGranted
         };
      }
   }
}

