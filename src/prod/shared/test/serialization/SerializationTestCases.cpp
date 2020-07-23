// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "SerializationTests.h"

//
// A table containing information of all test cases.
//

const KU_TEST_ENTRY gs_KuTestCases[] =
{
    {   L"BasicSerializationTest",                  BasicSerializationTest,                     L"CIT", L"Single object"},
    {   L"BasicSerializationWithChildTest",         BasicSerializationWithChildTest,            L"CIT", L"Child object"},
    {   L"BasicVersioningChildReadBaseTest",        BasicVersioningChildReadBaseTest,           L"CIT", L"Read base to child"},
    {   L"BasicVersioningChildTest",                BasicVersioningChildTest,                   L"CIT", L"Child is the new version"},
    {   L"BasicVersioningChild2Test",               BasicVersioningChild2Test,                  L"CIT", L"Same as BasicVersioningChildTest but it is the 2nd child in the inheritance tree"},
    {   L"BasicNestedObjectTest",                   BasicNestedObjectTest,                      L"CIT", L"Nested object"},
    {   L"BasicObjectWithPointersTest",             BasicObjectWithPointersTest,                L"CIT", L"Object with pointers"},
    {   L"PolymorphicObjectChildTest",              PolymorphicObjectChildTest,                 L"CIT", L"Polymorphics objects and deserializing as a pointer"},
    {   L"BasicObjectVersioningV1ToV2Test",         BasicObjectVersioningV1ToV2Test,            L"CIT", L"V1 to V2 test"},
    {   L"BasicObjectVersioningV2ToV1ToV2Test",     BasicObjectVersioningV2ToV1ToV2Test,        L"CIT", L"V2 to V1 to V2 test"},
    {   L"ObjectWithArraysVersioningV1ToV2Test",    ObjectWithArraysVersioningV1ToV2Test,       L"CIT", L"V1 to V2 with arrays"},
    {   L"ObjectWithArraysVersioningV2ToV1ToV2Test",ObjectWithArraysVersioningV2ToV1ToV2Test,   L"CIT", L"V2 to V1 to V2 with arrays"},
    {   L"InheritedObjectV2ToV1ToV2Test",           InheritedObjectV2ToV1ToV2Test,              L"CIT", L"Inherited objects with versioning"},
    {   L"BasicRvdObjectTest",                      BasicRvdObjectTest,                         L"CIT", L"Test rvd features"},
    {   L"TreeObjectTest",                          TreeObjectTest,                             L"CIT", L"Test a complicated tree object"},
    {   L"TreeObjectMixedVersionsTest",             TreeObjectMixedVersionsTest,                L"CIT", L"Test a complicated tree object with different versioned nodes"},
    {   L"BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy", BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy, L"CIT", L"Make sure errors are returned for unknown byte arrays no copy"}
};

const ULONG gs_KuTestCasesSize = ARRAYSIZE(gs_KuTestCases);

//TE.exe TestLoader.dll /p:c=RunTests /p:target=user /p:dll=SerializationUser /p:configfile=Table:SerializationTestInput.xml#Test /p:categories=CIT
