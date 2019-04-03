// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

/*
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif
*/

NTSTATUS BasicSerializationTest();

NTSTATUS BasicSerializationWithChildTest();

NTSTATUS BasicVersioningChildReadBaseTest();

NTSTATUS BasicVersioningChildTest();

NTSTATUS BasicVersioningChild2Test();

NTSTATUS BasicNestedObjectTest();

NTSTATUS BasicObjectWithPointersTest();

NTSTATUS PolymorphicObjectTest();

NTSTATUS PolymorphicObjectChildTest();

NTSTATUS BasicObjectVersioningV1ToV2Test();

NTSTATUS BasicObjectVersioningV2ToV1ToV2Test();

NTSTATUS ObjectWithArraysVersioningV1ToV2Test();

NTSTATUS ObjectWithArraysVersioningV2ToV1ToV2Test();

NTSTATUS InheritedObjectV2ToV1ToV2Test();

NTSTATUS BasicRvdObjectTest();

NTSTATUS TreeObjectTest();

NTSTATUS TreeObjectMixedVersionsTest();

NTSTATUS BadObjectVersioningV2ToV1WithUnknownByteArrayNoCopy();

