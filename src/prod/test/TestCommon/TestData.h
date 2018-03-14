// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class TestData
    {
    public:
        TestData();
        TestData(std::wstring const& result);
        TestData(std::vector<TestData> const& collection);
        TestData(std::map<std::wstring,TestData> const& map);

        __declspec (property(get=getResult)) std::wstring const& Result;
        std::wstring const& getResult() const;

        TestData const& operator[](int index) const;
        __declspec (property(get=getSize)) size_t Size;
        size_t getSize() const;

        TestData const& operator[](std::wstring key) const;
        bool ContainsKey(std::wstring const& key) const;

        enum Type
        {
            Invalid,
            Value,
            Collection,
            Map
        };

    private:
        Type type_;
        std::wstring result_;
        std::vector<TestData> collection_;
        std::map<std::wstring,TestData> map_;
    };
};
