// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

// currently windows is not using latest boost. 
#ifdef PLATFORM_UNIX
#define sealed sealed_
#define BOOST_TEST_MODULE My Test 
#include <boost/test/included/unit_test.hpp>
#else
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#endif 

