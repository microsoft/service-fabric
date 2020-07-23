/*++
Copyright (c) Microsoft Corporation

Module Name:    KTestAssert.h
Abstract:       Contains definitions for test asserts.
--*/

#pragma once

#include "ktl.h"

#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

class KTestAssert
{
public:
    static void IsTrue(bool condition)
    {
        TestAssert(condition);
    }

    static void IsTrue(BOOLEAN condition)
    {
        TestAssert(condition == TRUE);
    }

    static void IsTrue(NTSTATUS status)
    {
        TestAssert(NT_SUCCESS(status));
    }

    static void IsFalse(bool condition)
    {
        TestAssert(condition == false);
    }

    static void IsFalse(BOOLEAN condition)
    {
        TestAssert(condition == FALSE);
    }

    static void IsFalse(NTSTATUS status)
    {
        TestAssert(NT_SUCCESS(status) == false);
    }

    template <class T>
    static void AreEqual(T expected, T actual)
    {
        TestAssert(expected == actual);
    }

private:
    static void TestAssert(bool condition)
    {
        KInvariant(condition);
    }
};