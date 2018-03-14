// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      enum StoreBehavior : byte
      {
          //
          // Single version store.
          //
          SingleVersion = 0,

          // 
          // Multi-version store.
          //
          MultiVersion = 1,

          //
          // All version store.
          //
          Historical = 2,
      };
   }
}
