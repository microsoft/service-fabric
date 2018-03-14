// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
//#include "WexTestClass.h"
#include "Common/TextWriter.h"


#include <boost/test/unit_test.hpp>

int CopyCount = 0;

//namespace Common
//{
    class TestTextWriter
    {
    public:
        //TEST_CLASS(TestTextWriter)

        //TEST_METHOD(SmokeTest)

    public:
        class Foo
        {
        public:
            Foo(int id)
                : id_(id)
            {
            }

            Foo(Foo const & rhs)
                : id_(rhs.id_)
            {
                CopyCount++;
            }

            Foo & Foo::operator = (Foo const & rhs)
            {
                id_ = rhs.id_;
                CopyCount++;
                return *this;
            }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w.Write("Foo: {0}", id_);
            }

        private:
            int id_;
        };
    };

    BOOST_FIXTURE_TEST_SUITE(Common, TestTextWriter)

    BOOST_AUTO_TEST_CASE(SmokeTest)
    {
        unsigned char b = 137;
        std::wstring result = wformatString("{0:04x}", b);
        BOOST_REQUIRE(result == L"0089");
        result = wformatString("{0}", b);
        BOOST_REQUIRE(result == L"137");
        Foo f(123);
        result = wformatString("{0} {1}", f);
        BOOST_REQUIRE(result == L"Foo: 123 >> 1 ** insert index too big <<");
        BOOST_REQUIRE(CopyCount == 0);
    }

    BOOST_AUTO_TEST_SUITE_END()
//}
