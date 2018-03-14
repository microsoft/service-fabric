// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class Verify
            {
            public:

                template<typename T>
                static void Asserts(T functor, std::wstring const & msg)
                {
                    Asserts(functor, msg, "");
                }

                template<typename T>
                static void Asserts(T functor, std::wstring const & msg, std::string const & expectedAssertContents)
                {
                    try
                    {
                        functor();
                        Verify::Fail(msg);
                    }
                    catch (std::system_error const & e)
                    {
                        if (expectedAssertContents.empty())
                        {
                            return;
                        }

                        auto actualAssertContents = std::string(e.what());
                        Verify::IsTrue(Common::StringUtility::ContainsCaseInsensitive(actualAssertContents, expectedAssertContents), Common::wformatString("Assert text '{0}' does not contain '{1}'", actualAssertContents, expectedAssertContents));
                    }
                }


                static void Fail(std::wstring const & msg)
                {
                    std::wstring text = Common::wformatString("Verify: Failfast. {0}", msg);
                    TestLog::WriteError(text.c_str());
                }

# pragma region boolean

                static void IsTrue(bool expr)
                {
                    IsTrue(expr, L"");
                }

                static void IsTrue(bool expr, std::wstring const & msg)
                {
                    if (expr)
                    {
                        std::wstring text = Common::wformatString("Verify: IsTrue. {0}", msg);
                        TestLog::WriteInfo(text.c_str());
                    }
                    else
                    {
                        std::wstring text = Common::wformatString("Verify: IsTrue. {0}", msg);
                        TestLog::WriteError(text.c_str());
                    }
                }

                static void IsTrueLogOnError(bool expr)
                {
                    IsTrueLogOnError(expr, L"");
                }

                static void IsTrueLogOnError(bool expr, std::wstring const & msg)
                {
                    if (!expr)
                    {
                        std::wstring text = Common::wformatString("Verify: IsTrue. {0}", msg);
                        TestLog::WriteError(text.c_str());
                    }
                }

# pragma endregion

# pragma region Equality

                template<typename T, typename U>
                static void AreEqual(T const & expected, U const & actual)
                {
                    AreEqual<T, U>(expected, actual, false, L"");
                }

                template<typename T, typename U>
                static void AreEqualLogOnError(T const & expected, U const & actual)
                {
                    AreEqual<T, U>(expected, actual, true, L"");
                }

                template<typename T, typename U>
                static void AreEqual(T const & expected, U const & actual, std::wstring const & message)
                {
                    AreEqual(expected, actual, false, message);
                }

                template<typename T, typename U>
                static void AreEqualLogOnError(T const & expected, U const & actual, std::wstring const & message)
                {
                    AreEqual(expected, actual, true, message);
                }

                template<>
                static void AreEqual(std::wstring const & expected, std::wstring const & actual, std::wstring const & message)
                {
                    AreEqual(expected, actual, false, message);
                }

                template<>
                static void AreEqualLogOnError(std::wstring const & expected, std::wstring const & actual, std::wstring const & message)
                {
                    AreEqual(expected, actual, true, message);
                }

# pragma endregion

# pragma region Contains

                static std::wstring GetMessageForStringContains(std::wstring const & bigStr, std::wstring const & smallStr, std::wstring const & message)
                {
                    return Common::wformatString("{0}\r\nDid not find\r\n{1}\r\nin\r\n{2}", message, smallStr, bigStr);
                }

                static void StringContainsCaseInsensitive(std::wstring const & bigStr, std::wstring const & smallStr, std::wstring const & message)
                {
                    Verify::IsTrue(Common::StringUtility::ContainsCaseInsensitive(bigStr, smallStr), GetMessageForStringContains(bigStr, smallStr, message));
                }

                static void StringContains(std::wstring const & bigStr, std::wstring const & smallStr, std::wstring const & message)
                {
                    Verify::IsTrue(Common::StringUtility::Contains(bigStr, smallStr), GetMessageForStringContains(bigStr, smallStr, message));
                }
#pragma endregion

# pragma region Vector

                template<typename T>
                static void VectorStrict(std::vector<T> const & v)
                {
                    VERIFY_IS_TRUE(v.empty());
                }

                template<typename T>
                static void VectorStrictAsString(std::vector<T> const & expected, std::vector<T> const & actual)
                {
                    Verify::AreEqualLogOnError(expected.size(), actual.size(), L"Size");
                    if (expected.size() != actual.size())
                    {
                        return;
                    }

                    for (size_t i = 0; i < expected.size(); ++i)
                    {
                        Verify::AreEqualLogOnError(expected[i], actual[i], Common::wformatString("Element at index {0}", i));
                    }
                }

                template<typename T>
                static void VectorStrict(std::vector<T> const & expected, std::vector<T> const & actual)
                {
                    VERIFY_ARE_EQUAL(expected.size(), actual.size(), L"Size");

                    for (size_t i = 0; i < expected.size(); ++i)
                    {
                        VERIFY_ARE_EQUAL(expected[i], actual[i], Common::wformatString("Element at index {0}", i).c_str());
                    }
                }

                template<typename T>
                static void VectorStrict(std::vector<T> const & v, T const & el1)
                {
                    VERIFY_ARE_EQUAL(1u, v.size());
                    VERIFY_ARE_EQUAL(el1, v[0]);
                }

                template<typename T>
                static void VectorStrict(std::vector<T> const & v, T const & el1, T const & el2)
                {
                    VERIFY_ARE_EQUAL(2u, v.size());
                    VERIFY_ARE_EQUAL(el1, v[0]);
                    VERIFY_ARE_EQUAL(el2, v[1]);
                }

                template<typename T>
                static void VectorStrict(std::vector<T> const & v, T const & el1, T const & el2, T const & el3)
                {
                    VERIFY_ARE_EQUAL(3u, v.size());
                    VERIFY_ARE_EQUAL(el1, v[0]);
                    VERIFY_ARE_EQUAL(el2, v[1]);
                    VERIFY_ARE_EQUAL(el3, v[2]);
                }

                template<typename T>
                static void VectorStrict(std::vector<T> const & v, T const & el1, T const & el2, T const & el3, T const & el4)
                {
                    VERIFY_ARE_EQUAL(4u, v.size());
                    VERIFY_ARE_EQUAL(el1, v[0]);
                    VERIFY_ARE_EQUAL(el2, v[1]);
                    VERIFY_ARE_EQUAL(el3, v[2]);
                    VERIFY_ARE_EQUAL(el4, v[3]);
                }

                template<typename T, typename F, typename E>
                static void VectorStrict(std::vector<T> const & v, F func, E const & el1)
                {
                    VERIFY_ARE_EQUAL(1u, v.size());
                    VERIFY_ARE_EQUAL(el1, func(v[0]));
                }

                template<typename T, typename F, typename E>
                static void VectorStrict(std::vector<T> const & v, F func, E const & el1, E const & el2)
                {
                    VERIFY_ARE_EQUAL(2u, v.size());
                    VERIFY_ARE_EQUAL(el1, func(v[0]));
                    VERIFY_ARE_EQUAL(el2, func(v[1]));
                }

                template<typename T, typename F, typename E>
                static void VectorStrict(std::vector<T> const & v, F func, E const & el1, E const & el2, E const & el3)
                {
                    VERIFY_ARE_EQUAL(3u, v.size());
                    VERIFY_ARE_EQUAL(el1, func(v[0]));
                    VERIFY_ARE_EQUAL(el2, func(v[1]));
                    VERIFY_ARE_EQUAL(el3, func(v[2]));
                }

                template<typename T, typename F, typename E>
                static void VectorStrict(std::vector<T> const & v, F func, E const & el1, E const & el2, E const & el3, E const & el4)
                {
                    VERIFY_ARE_EQUAL(4u, v.size());
                    VERIFY_ARE_EQUAL(el1, func(v[0]));
                    VERIFY_ARE_EQUAL(el2, func(v[1]));
                    VERIFY_ARE_EQUAL(el3, func(v[2]));
                    VERIFY_ARE_EQUAL(el4, func(v[3]));
                }


                template<typename T>
                static void Vector(std::vector<T> const & v)
                {
                    VERIFY_IS_TRUE(v.empty());
                }

                template<typename T>
                static void Vector(std::vector<T> const & v, T const & el1)
                {
                    VERIFY_ARE_EQUAL(1u, v.size());
                    VERIFY_ARE_EQUAL(el1, v[0]);
                }

                template<typename T>
                static void Vector(std::vector<T> const & v, T const & el1, T const & el2)
                {
                    VERIFY_ARE_EQUAL(2u, v.size());

                    std::vector<T const *> expected;
                    expected.push_back(&el1);
                    expected.push_back(&el2);

                    Vector(v, expected);
                }

                template<typename T>
                static void Vector(std::vector<T> const & v, T const & el1, T const & el2, T const & el3)
                {
                    VERIFY_ARE_EQUAL(3u, v.size());

                    std::vector<T const *> expected;
                    expected.push_back(&el1);
                    expected.push_back(&el2);
                    expected.push_back(&el3);

                    Vector(v, expected);
                }

                template<typename T>
                static void Vector(std::vector<T> const & v, T const & el1, T const & el2, T const & el3, T const & el4)
                {
                    VERIFY_ARE_EQUAL(4u, v.size());

                    std::vector<T const *> expected;
                    expected.push_back(&el1);
                    expected.push_back(&el2);
                    expected.push_back(&el3);
                    expected.push_back(&el4);

                    Vector(v, expected);

                }

                template<typename T>
                static void Vector(std::vector<T> const & v, std::vector<T> const & expected)
                {
                    Verify::AreEqual(expected.size(), v.size(), L"Verify::Vector size");
                    VERIFY_ARE_EQUAL(expected.size(), v.size());

                    for (auto it = v.cbegin(); it != v.cend(); ++it)
                    {
                        auto j = find_if(expected.cbegin(), expected.cend(), [it](T const & jt)
                        {
                            return *it == jt;
                        });


                        if (j == expected.cend())
                        {
                            std::wstring error = Common::wformatString("Failed to find {0}", *it);
                            TestLog::WriteError(error.c_str());
                        }

                        VERIFY_IS_TRUE(j != expected.cend());
                    }
                }

# pragma endregion

# pragma region Map
                template<typename K, typename T>
                static void Map(std::map<K, std::vector<T>> const & actual, std::map<K, std::vector<T>> const & expected)
                {
                    VERIFY_ARE_EQUAL(expected.size(), actual.size());

                    auto itActual = actual.begin();
                    auto itExpected = expected.begin();

                    while (itActual != actual.end())
                    {
                        VERIFY_ARE_EQUAL(itActual->first, itExpected->first);
                        Vector<T>(itActual->second, itExpected->second);
                        ++itActual;
                        ++itExpected;
                    }
                }
# pragma endregion

# pragma region Compare

                template<typename T>
                static void Compare(T const & expected, T const & actual)
                {
                    Compare(expected, actual, L"");
                }

                template<typename T>
                static void Compare(T const & expected, T const & actual, std::wstring const & msg)
                {
                    comparer_traits<T>::Compare(expected, actual, msg);
                }

                template<typename T, typename U>
                static void Compare(T const & expected, U const & actual, std::wstring const & msg)
                {
                    comparer_traits2<T, U>::Compare(expected, actual, msg);
                }

                template<typename T>
                static void Compare(std::wstring const & expected, T const & actual)
                {
                    Compare(expected, actual, L"");
                }

                template<typename T>
                static void Compare(std::wstring const & expected, T const & actual, std::wstring const & msg)
                {
                    Compare(expected, actual, StateManagement::ReadContext(), msg);
                }

                template<typename T>
                static void Compare(std::wstring const & expected, T const & actual, StateManagement::ReadContext const & context)
                {
                    Compare(expected, actual, context, L"");
                }

                template<typename T>
                static void Compare(std::wstring const & expected, T const & actual, StateManagement::ReadContext const & context, std::wstring const & msg)
                {
                    T expectedObj = StateManagement::Reader::ReadHelper<T>(expected, context);
                    Compare(expectedObj, actual, msg);
                }

# pragma endregion

            private:

# pragma region Equality

                template<typename T, typename U>
                static void AreEqual(T const & expected, U const & actual, bool logOnError, std::wstring const & message)
                {
                    if (expected == actual)
                    {
                        if (!logOnError)
                        {
                            std::wstring text = Common::wformatString("Verify: AreEqual({0}, {1}). {2}", expected, actual, message);
                            TestLog::WriteInfo(text.c_str());
                        }
                    }
                    else
                    {
                        std::wstring text = Common::wformatString("Verify: AreEqual({0}, {1}). {2}", expected, actual, message);
                        TestLog::WriteError(text.c_str());
                    }
                }

                template<>
                static void AreEqual(std::wstring const & expected, std::wstring const & actual, bool logOnError, std::wstring const & message)
                {
                    if (expected.size() != actual.size())
                    {
                        std::wstring text = Common::wformatString("Verify: AreEqual [size mismatch] ({0}, {1}). {2}", expected, actual, message);
                        TestLog::WriteError(text.c_str());
                    }
                    else
                    {
                        bool equal = true;
                        size_t unequalIndex = 0;
                        for (size_t i = 0; i < expected.size(); i++)
                        {
                            if (expected[i] != actual[i])
                            {
                                unequalIndex = i;
                                equal = false;
                                break;
                            }
                        }

                        if (equal)
                        {
                            if (!logOnError)
                            {
                                std::wstring text = Common::wformatString("Verify: AreEqual({0}, {1}). {2}", expected, actual, message);
                                TestLog::WriteInfo(text.c_str());
                            }
                        }
                        else
                        {
                            std::wstring text = Common::wformatString("Verify: AreEqual ({0}, {1}). {2}\r\nMatch failed at pos: {3}. {4} {5}", expected, actual, message, unequalIndex, expected[unequalIndex], actual[unequalIndex]);
                            TestLog::WriteError(text.c_str());
                        }
                    }
                }

# pragma endregion

                template<typename T>
                static void Vector(std::vector<T> const & v, std::vector<T const *> const & expected)
                {
                    VERIFY_ARE_EQUAL(expected.size(), v.size());

                    for (auto it = v.cbegin(); it != v.cend(); ++it)
                    {
                        auto j = find_if(expected.cbegin(), expected.cend(), [it](T const * jt)
                        {
                            return *it == *jt;
                        });


                        if (j == expected.cend())
                        {
                            std::wstring error = Common::wformatString("Failed to find {0}", *it);
                            TestLog::WriteError(error.c_str());
                        }

                        VERIFY_IS_TRUE(j != expected.cend());
                    }
                }

# pragma region comparer_traits

                template<typename T>
                struct comparer_traits
                {
                    static void Compare(T const & expected, T const & actual, std::wstring const & msg)
                    {
                        std::wstring expectedBodyStr = Common::wformatString(expected);
                        std::wstring actualBodyStr = Common::wformatString(actual);

                        Verify::AreEqualLogOnError(expectedBodyStr, actualBodyStr, msg);
                    }
                };

                template<typename T, typename U>
                struct comparer_traits2
                {
                    static void Compare(T const & expected, U const & actual, std::wstring const & msg)
                    {
                        std::wstring expectedBodyStr = Common::wformatString(expected);
                        std::wstring actualBodyStr = Common::wformatString(actual);

                        Verify::AreEqualLogOnError(expectedBodyStr, actualBodyStr, msg);
                    }
                };

                template<>
                struct comparer_traits < Reliability::LoadBalancingComponent::LoadOrMoveCostDescription >
                {
                    static void Compare(Reliability::LoadBalancingComponent::LoadOrMoveCostDescription const & expected, Reliability::LoadBalancingComponent::LoadOrMoveCostDescription const & actual, std::wstring const &)
                    {
                        Verify::AreEqualLogOnError(expected.FailoverUnitId, actual.FailoverUnitId, L"FT id does not match");
                        Verify::AreEqualLogOnError(expected.IsStateful, actual.IsStateful, L"IsStateful does not match");
                        Verify::AreEqualLogOnError(expected.ServiceName, actual.ServiceName, L"Service name does not match");
                        if (expected.IsStateful)
                        {
                            Vector<Reliability::LoadBalancingComponent::LoadMetricStats>(actual.PrimaryEntries, expected.PrimaryEntries);
                        }
                        Vector<Reliability::LoadBalancingComponent::LoadMetricStats>(actual.SecondaryEntries, expected.SecondaryEntries);
                        Map<Federation::NodeId, Reliability::LoadBalancingComponent::LoadMetricStats>(actual.SecondaryEntriesMap, expected.SecondaryEntriesMap);
                    }
                };

                template<>
                struct comparer_traits < Reliability::ReportLoadMessageBody >
                {
                    static void Compare(Reliability::ReportLoadMessageBody const & expected, Reliability::ReportLoadMessageBody const & actual, std::wstring const & msg)
                    {
                        auto expectedLoads = expected.Reports;
                        auto actualLoads = actual.Reports;

                        Verify::AreEqualLogOnError(expectedLoads.size(), actualLoads.size(), L"Reports size does not match");

                        auto sortReportLoad = [](Reliability::LoadBalancingComponent::LoadOrMoveCostDescription a, Reliability::LoadBalancingComponent::LoadOrMoveCostDescription b) {
                            return a.FailoverUnitId < b.FailoverUnitId;
                        };
                        std::sort(expectedLoads.begin(), expectedLoads.end(), sortReportLoad);
                        std::sort(actualLoads.begin(), actualLoads.end(), sortReportLoad);

                        auto itActual = actualLoads.begin();
                        for (auto itExpected = expectedLoads.begin(); itExpected != expectedLoads.end(); ++itExpected, ++itActual)
                        {
                            comparer_traits<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription>::Compare(*itExpected, *itActual, msg);
                        }
                    }
                };

                template<>
                struct comparer_traits < ServiceModel::QueryResult >
                {
                    static void Compare(ServiceModel::QueryResult const & expectedR, ServiceModel::QueryResult const & actualR, std::wstring const &)
                    {
                        ServiceModel::QueryResult & expected = const_cast<ServiceModel::QueryResult &>(expectedR);
                        ServiceModel::QueryResult & actual = const_cast<ServiceModel::QueryResult &>(actualR);
                        Verify::AreEqualLogOnError(Common::wformatString(expected.QueryProcessingError), Common::wformatString(actual.QueryProcessingError), L"QueryResult: Common::ErrorCode");

                        if (!expected.QueryProcessingError.IsSuccess())
                        {
                            return;
                        }

                        Verify::AreEqualLogOnError(Common::wformatString(static_cast<int>(expected.ResultKind)), Common::wformatString(static_cast<int>(actual.ResultKind)), L"QueryResult: Kind");
                        if (expected.ResultKind != actual.ResultKind)
                        {
                            return;
                        }

                        ServiceModel::QueryResultListWrapperBase const & innerExpected = expected.Test_GetResultWrapperBase();
                        ServiceModel::QueryResultListWrapperBase const & innerActual = actual.Test_GetResultWrapperBase();
                        Verify::AreEqualLogOnError(Common::wformatString(static_cast<int>(innerExpected.ResultKind)), Common::wformatString(static_cast<int>(innerActual.ResultKind)), L"QueryResult:: Inner Result Kind");
                        if (innerExpected.ResultKind != innerActual.ResultKind)
                        {
                            return;
                        }

                        switch (innerExpected.ResultKind)
                        {
                        case ServiceModel::QueryResultHelpers::DeployedServiceReplica:
                            CompareListInner<ServiceModel::DeployedServiceReplicaQueryResult>(expected, actual);
                            break;

                        case ServiceModel::QueryResultHelpers::DeployedServiceReplicaDetail:
                            CompareValueInner<ServiceModel::DeployedServiceReplicaDetailQueryResult>(expected, actual);
                            break;

                        default:
                            Verify::Fail(Common::wformatString("Unknown type {0}", static_cast<int>(innerExpected.ResultKind)));
                            break;
                        };
                    }

                private:
                    template<typename InnerT>
                    static void CompareListInner(ServiceModel::QueryResult & expected, ServiceModel::QueryResult & actual)
                    {
                        std::vector<InnerT> expectedItems, actualItems;

                        expected.MoveList(expectedItems);
                        actual.MoveList(actualItems);
                        Verify::AreEqualLogOnError(expectedItems.size(), actualItems.size(), L"Item Size");

                        if (expectedItems.size() != actualItems.size())
                        {
                            return;
                        }

                        for (size_t i = 0; i < actualItems.size(); i++)
                        {
                            Verify::AreEqualLogOnError(Common::wformatString(expectedItems[i]), Common::wformatString(actualItems[i]), Common::wformatString("Item at {0}", i));
                        }
                    }

                    template<typename InnerT>
                    static void CompareValueInner(ServiceModel::QueryResult & expected, ServiceModel::QueryResult & actual)
                    {
                        InnerT expectedValue, actualValue;

                        expected.MoveItem(expectedValue);
                        actual.MoveItem(actualValue);

                        Verify::AreEqualLogOnError(Common::wformatString(expectedValue), Common::wformatString(actualValue), L"QueryResult::Item Value");
                    }
                };

                template<>
                struct comparer_traits < ServiceModel::HealthReport >
                {
                    static void Compare(ServiceModel::HealthReport const & expected, ServiceModel::HealthReport const & actual, std::wstring const &)
                    {
                        CompareAttributeList(expected.AttributeInfo.Attributes, actual.AttributeInfo.Attributes);

                        Verify::AreEqualLogOnError(expected.Description, actual.Description, L"HealthReport: Description");
                        Verify::AreEqualLogOnError(expected.Property, actual.Property, L"HealthReport: Property");
                        Verify::AreEqualLogOnError(expected.Kind, actual.Kind, L"HealthReport: Kind");
                        Verify::AreEqualLogOnError(expected.EntityPropertyId, actual.EntityPropertyId, L"HealthReport: EntityPropertyId");
                        Verify::AreEqualLogOnError(expected.State, actual.State, L"HealthReport: State");
                        Verify::AreEqualLogOnError(expected.SourceId, actual.SourceId, L"HealthReport: SourceID");
                    }

                private:
                    static void CompareAttributeList(
                        std::map<std::wstring, std::wstring> const & expected,
                        std::map<std::wstring, std::wstring> const & actual)
                    {
                        // Verify Attribute List
                        Verify::AreEqualLogOnError(expected.size(), actual.size(), L"HealthReport: Attribute count");

                        if (!expected.empty())
                        {
                            Verify::AreEqualLogOnError(2, expected.size(), L"HealthReport: expected 2 items - node id and node instance");
                            CompareAttribute(expected, actual, ServiceModel::HealthAttributeNames::NodeId);
                            CompareAttribute(expected, actual, ServiceModel::HealthAttributeNames::NodeInstanceId);
                        }
                    }

                    static void CompareAttribute(
                        std::map<std::wstring, std::wstring> const & expected,
                        std::map<std::wstring, std::wstring> const & actual,
                        std::wstring const & keyName)
                    {
                        auto expectedValue = expected.find(keyName)->second;
                        auto actualValue = actual.find(keyName)->second;
                        Verify::AreEqualLogOnError(
                            expectedValue,
                            actualValue,
                            Common::wformatString("HealthReport: Attribute {0} did not match", keyName));
                    }
                };

# pragma endregion
            };
        }
    }
}
