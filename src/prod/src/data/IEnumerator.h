// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    template <typename T>
    class IEnumerator : public KObject<IEnumerator<T>>, public KShared<IEnumerator<T>>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(IEnumerator);

    public:

        virtual T Current() = 0;
        virtual bool MoveNext() = 0;
    };

    template <typename T>
    IEnumerator<T>::IEnumerator()
    {
    }

    template <typename T>
    IEnumerator<T>::~IEnumerator()
    {
    }
}
