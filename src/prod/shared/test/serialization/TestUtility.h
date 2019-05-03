// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

template <class T>
bool IsPointerEqual(T * left, T * right)
{
    if (left == nullptr && right == nullptr)
    {
        return true;
    }
    else if (left == nullptr || right== nullptr)
    {
        return false;
    }

    return (*left == *right);
}

template <class T>
bool IsArrayEqual(KArray<T> const & left, KArray<T> const & right)
{
    if (left.Count() != right.Count())
    {
        return false;
    }

    for (ULONG i = 0; i < left.Count(); ++i)
    {
        if (!(left[i] == right[i]))
        {
            return false;
        }
    }

    return true;
}

template <class T>
bool IsArrayEqual(KArray<KUniquePtr<T>> const & left, KArray<KUniquePtr<T>> const & right)
{
    if (left.Count() != right.Count())
    {
        return false;
    }

    for (ULONG i = 0; i < left.Count(); ++i)
    {
        if (!IsPointerEqual<T>(left[i], right[i]))
        {
            return false;
        }
    }

    return true;
}

template <class T>
NTSTATUS FillArray(KArray<T> & arr, T startValue, T size)
{
    NTSTATUS status = STATUS_SUCCESS;

    for (T i = 0; i < size; ++i)
    {
        status = arr.Append(startValue + i);

        if (NT_ERROR(status)) return status;
    }

    return status;
}

bool AreBuffersEqual(KArray<FabricIOBuffer> const & left, KArray<FabricIOBuffer> const & right)
{
    if (left.Count() != right.Count())
    {
        return false;
    }

    for (ULONG i = 0; i < left.Count(); ++i)
    {
        if (left[i].length != right[i].length)
        {
            return false;
        }


        if (memcmp(left[i].buffer, right[i].buffer, left[i].length) != 0)
        {
            return false;
        }
    }

    return true;
}

