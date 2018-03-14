// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      //
      // Metdata file proprties.
      //
      namespace PropertyId
      {
         enum Enum
         {
            MetadataHandle = 1,
            DataFileCount = 2,
            CheckpointLSN = 3,
         };
      }
   }
}
