// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class UniquePtr : private DenyCopy
    {
    public:
        template <typename T>
        static std::unique_ptr<T> Create()
        {
            return std::unique_ptr<T>(new T());
        }

        template <typename T, typename T1>
        static std::unique_ptr<T> Create(T1 && t1)
        {
            return std::unique_ptr<T>(new T(std::forward<T1>(t1)));
        }

        template <typename T, typename T1, typename T2>
        static std::unique_ptr<T> Create(T1 && t1, T2 && t2)
        {
            return std::unique_ptr<T>(new T(std::forward<T1>(t1), std::forward<T2>(t2)));
        }

    private:
        UniquePtr()
        {
        }

        ~UniquePtr()
        {
        }
    };
}
