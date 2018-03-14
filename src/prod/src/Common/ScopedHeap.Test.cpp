// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReferenceArray.h"
#include "ReferencePointer.h"
#include "ScopedHeap.h"

namespace Common
{
    struct TEST_APPLICATION_TYPE_PARAMETER
    {
        LPCWSTR Name;
        LPCWSTR Value;
    };

    struct TEST_APPLICATION_TYPE_PARAMETER_LIST
    {
        ULONG Count;
        TEST_APPLICATION_TYPE_PARAMETER * Items;
    };

    struct TEST_APPLICATION_DESCRIPTION
    {
        LPCWSTR ApplicationName;
        LPCWSTR ApplicationTypeName;
        LPCWSTR ApplicationTypeVersion;
        TEST_APPLICATION_TYPE_PARAMETER_LIST * ApplicationTypeParameterValues;
    };

    class TestScopedHeap
    {
    protected:
        ErrorCode InitializeDescription();
        std::wstring GenerateName(size_t index);
        std::wstring GenerateValue(size_t index);
    };

    BOOST_FIXTURE_TEST_SUITE(TestScopedHeapSuite,TestScopedHeap)

    BOOST_AUTO_TEST_CASE(CheckLifetime)
    {
        ErrorCode error = InitializeDescription();
        ASSERT_IFNOT(error.IsSuccess(), "Should not run out of memory during test case");
    }

    BOOST_AUTO_TEST_SUITE_END()

    ErrorCode TestScopedHeap::InitializeDescription()
    {
        ScopedHeap heap;

        std::wstring appName(L"app");
        std::wstring appTypeName(
            L"Application type name that is a bit longer than the application name, which was especially short. \
              It was, in fact, shorter than the threshhold where std::wstring allocates.  The idea is to verify \
              both strings that allocate and strings that don't can be transferred into the heap without messing \
              up the heap. \
              \
              Also, appTypeVersion (below) verifies that empty strings work as well.");
        std::wstring appTypeVersion(L"");
        size_t count = 1000U;

        auto description = heap.AddItem<TEST_APPLICATION_DESCRIPTION>();
        auto parameterList = heap.AddItem<TEST_APPLICATION_TYPE_PARAMETER_LIST>();
        auto parameters = heap.AddArray<TEST_APPLICATION_TYPE_PARAMETER>(count);

        if (!description || !parameterList || !parameters)
        {
            return ErrorCode(ErrorCodeValue::OutOfMemory);
        }

        description->ApplicationName = heap.AddString(appName);
        description->ApplicationTypeName = heap.AddString(appTypeName);
        description->ApplicationTypeVersion = heap.AddString(appTypeVersion);
        description->ApplicationTypeParameterValues = parameterList.GetRawPointer();

        if ((description->ApplicationName == NULL) ||
            (description->ApplicationTypeName == NULL) ||
            (description->ApplicationTypeVersion == NULL))
        {
            return ErrorCode(ErrorCodeValue::OutOfMemory);
        }

        parameterList->Count = static_cast<ULONG>(parameters.GetCount());
        parameterList->Items = parameters.GetRawArray();

        // Add 1000 strings of increasing length
        for (size_t i=0; i<count; i++)
        {
            LPCWSTR x = heap.AddString(GenerateName(i));
            parameters[i].Name = x;
            parameters[i].Value = heap.AddString(GenerateValue(i));

            if ((parameters[i].Name == NULL) ||
                (parameters[i].Value == NULL))
            {
                return ErrorCode(ErrorCodeValue::OutOfMemory);
            }
        }

        // This is the value that would be returned by the ComObject containing heap.
        TEST_APPLICATION_DESCRIPTION * rawDescription = description.GetRawPointer();

        // TODO: verify that the contents of the raw data structures are all valid.
        ASSERT_IFNOT(rawDescription != NULL, "rawDescription is null");
        ASSERT_IFNOT(rawDescription->ApplicationName == appName, "mismatch: appName");
        ASSERT_IFNOT(rawDescription->ApplicationTypeName == appTypeName, "mismatch: appTypeName");
        ASSERT_IFNOT(rawDescription->ApplicationTypeVersion == appTypeVersion, "mismatch: appTypeVersion");

        ASSERT_IFNOT(rawDescription->ApplicationTypeParameterValues->Count == static_cast<ULONG>(count), "mismatch: count");
        for (size_t i=0; i<count; i++)
        {
            ASSERT_IFNOT(
                rawDescription->ApplicationTypeParameterValues->Items[i].Name == GenerateName(i),
                "mismatch: name {0}",
                i);
            ASSERT_IFNOT(
                rawDescription->ApplicationTypeParameterValues->Items[i].Value == GenerateValue(i),
                "mismatch: value {0}",
                i);
        }

        return ErrorCode::Success();
    }

    std::wstring TestScopedHeap::GenerateName(size_t index)
    {
        std::wstring x = std::wstring(index, L'n');
        return std::wstring(index, L'n');
    }

    std::wstring TestScopedHeap::GenerateValue(size_t index)
    {
        return std::wstring(index, L'v');
    }
}
