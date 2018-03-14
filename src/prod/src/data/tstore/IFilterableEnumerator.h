// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template <typename T>
        class IFilterableEnumerator : public IEnumerator<T>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(IFilterableEnumerator);

        public:
            virtual bool MoveTo(T const & item) = 0;
        };

        template <typename T>
        IFilterableEnumerator<T>::IFilterableEnumerator()
        {
        }

        template <typename T>
        IFilterableEnumerator<T>::~IFilterableEnumerator()
        {
        }
    }
}
