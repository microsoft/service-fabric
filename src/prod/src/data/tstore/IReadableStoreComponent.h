// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>
      interface IReadableStoreComponent
      {
         K_SHARED_INTERFACE(IReadableStoreComponent)

      public:
         virtual bool ContainsKey(__in TKey& key) const = 0;
         virtual KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key) const = 0;
      };
   }
}
