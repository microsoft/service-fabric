// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_NO_MAIN

#include <boost/test/unit_test.hpp>

bool initBoost()
{
    return true;
}

int BOOST_TEST_CALL_DECL
main(int argc, char* argv[])
{
    return ::boost::unit_test::unit_test_main(&initBoost, argc, argv);

}
