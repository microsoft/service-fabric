//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      interface ISweepProvider
      {
         K_SHARED_INTERFACE(ISweepProvider)

      public:
         virtual LONG64 GetMemorySize() = 0;

         virtual LONG64 GetEstimatedKeySize() = 0;
      };
   }
}
