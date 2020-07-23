// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Utility.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

wstring const Utility::ParamDelimiter = L" ";
wstring const Utility::FieldDelimiter = L" ";
wstring const Utility::ItemDelimiter = L",";
wstring const Utility::PairDelimiter = L"/";
wstring const Utility::KeyValueDelimiter = L":";
wstring const Utility::ConfigDelimiter = L"=";
wstring const Utility::CommentInit = L"#";
wstring const Utility::SectionConfig = L"Config";
wstring const Utility::SectionStart = L"<";
wstring const Utility::SectionEnd = L">";
wstring const Utility::EmptyCharacter = L"-";
wstring const Utility::VectorStartDelimiter = L"{";
wstring const Utility::VectorDelimiter = L"}";

StringLiteral const Utility::TraceSource = "LBSimulator";

bool Utility::Bool_Parse(wstring const & input)
{
    if (StringUtility::AreEqualCaseInsensitive(input, L"true") ||
        StringUtility::AreEqualCaseInsensitive(input, L"yes") ||
        StringUtility::AreEqualCaseInsensitive(input, L"t") ||
        StringUtility::AreEqualCaseInsensitive(input, L"y"))
    {
        return true;
    }
    else if (
        StringUtility::AreEqualCaseInsensitive(input, L"false") ||
        StringUtility::AreEqualCaseInsensitive(input, L"no") ||
        StringUtility::AreEqualCaseInsensitive(input, L"f") ||
        StringUtility::AreEqualCaseInsensitive(input, L"n"))
    {
        return false;
    }
    else
    {
        Assert::CodingError("Value cannot be parsed into a boolean: {0}", input.c_str());
    }
}

StringCollection Utility::Split(wstring const& input, wstring const& delimiter)
{
    StringCollection results;
    StringUtility::Split(input, results, delimiter);
    return results;
}

StringCollection Utility::Split(wstring const& input, wstring const& delimiter, wstring const& emptyChar)
{
    // split the input and replace those "-" by empty
    StringCollection results;
    StringUtility::Split(input, results, delimiter);
    for (size_t i = 0; i < results.size(); ++i)
    {
        if (results[i] == emptyChar)
        {
            results[i].clear();
        }
    }

    return results;
}

StringCollection Utility::SplitWithParentheses(wstring const& input, wstring const& delimiter, wstring const& emptyChar)
{
    wstring allDelimiter(delimiter);

    allDelimiter.append(L"(");

    // split the input, don't split the ones within parentheses, and replace those "-" by empty
    StringCollection results;
    if (input.empty())
    {
        return results;
    }

    size_t startTokenPos = 0;
    size_t endTokenPos = 0;
    int level = 0;

    while (wstring::npos != endTokenPos)
    {
        if (level == 0)
        {
            endTokenPos = input.find_first_of(allDelimiter, endTokenPos);

            if (wstring::npos != endTokenPos)
            {
                if (input[endTokenPos] == L'(')
                {
                    ++level;
                    ++endTokenPos;
                }
                else
                {
                    wstring token = input.substr(startTokenPos, endTokenPos - startTokenPos);
                    if (token == emptyChar)
                    {
                        token.clear();
                    }
                    results.push_back(token);

                    ++endTokenPos;
                    startTokenPos = endTokenPos;
                }
            }
        }
        else
        {
            ASSERT_IFNOT(level > 0, "Invalid level {0}", level);
            endTokenPos = input.find_first_of(L"()", endTokenPos);

            if (wstring::npos != endTokenPos)
            {
                if (input[endTokenPos] == L'(')
                {
                    ++level;
                }
                else if (level > 0)
                {
                    --level;
                }
            }

            ++endTokenPos;
        }
    }

    if (startTokenPos < input.size())
    {
        wstring token = input.substr(startTokenPos);
        if (token == emptyChar)
        {
            token.clear();
        }
        results.push_back(token);
    }

    return results;
}

void Utility::LoadFile(wstring const & fileName, wstring & fileTextW)
{
    File fileReader;
    auto error = fileReader.TryOpen(fileName, FileMode::OpenOrCreate, FileAccess::Read, FileShare::ReadWrite);
    if (!error.IsSuccess())
    {
        RealConsole().WriteLine("Failed to open {0}: {1}", fileName, error);
        return;
    }

    size_t size = static_cast<size_t>(fileReader.size());
    string filetext;
    filetext.resize(size);
    fileReader.Read(&filetext[0], static_cast<int>(size));
    fileReader.Close();
    StringWriter(fileTextW).Write(filetext);
}

wstring Utility::ReadLine(wstring const & text, size_t & pos)
{
    static wstring lineSeparator = L"\n\r";
    size_t start = text.find_first_not_of(lineSeparator, pos);
    if (start == wstring::npos)
    {
        return wstring();
    }

    pos = text.find_first_of(lineSeparator, start);
    if (pos == wstring::npos)
    {
        return text.substr(start);
    }
    else
    {
        return text.substr(start, pos - start);
    }
}

void Utility::UpdateConfigValue(map<wstring, wstring> & configMap, Reliability::LoadBalancingComponent::PLBConfig & config)
{
    for (map<wstring, wstring>::iterator it = configMap.begin(); it != configMap.end(); it++)
    {
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DummyPLBEnabled"))
        {
            config.DummyPLBEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseCRMPublicName"))
        {
            config.UseCRMPublicName = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"TraceCRMReasons"))
        {
            config.TraceCRMReasons = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ValidatePlacementConstraint"))
        {
            config.ValidatePlacementConstraint = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementConstraintValidationCacheSize"))
        {
            config.PlacementConstraintValidationCacheSize = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintViolationReportingPolicy"))
        {
            config.ConstraintViolationReportingPolicy = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"VerboseHealthReportLimit"))
        {
            config.VerboseHealthReportLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintViolationHealthReportLimit"))
        {
            config.ConstraintViolationHealthReportLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedConstraintViolationHealthReportLimit"))
        {
            config.DetailedConstraintViolationHealthReportLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedVerboseHealthReportLimit"))
        {
            config.DetailedVerboseHealthReportLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConsecutiveDroppedMovementsHealthReportLimit"))
        {
            config.ConsecutiveDroppedMovementsHealthReportLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedNodeListLimit"))
        {
            config.DetailedNodeListLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedPartitionListLimit"))
        {
            config.DetailedPartitionListLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedDiagnosticsInfoListLimit"))
        {
            config.DetailedDiagnosticsInfoListLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DetailedMetricListLimit"))
        {
            config.DetailedMetricListLimit = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBHealthEventTTL"))
        {
            config.PLBHealthEventTTL = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBRefreshInterval"))
        {
            config.PLBRefreshInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        else if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBRefreshGap"))
        {
            config.PLBRefreshGap = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MinPlacementInterval"))
        {
            config.MinPlacementInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MinConstraintCheckInterval"))
        {
            config.MinConstraintCheckInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MinLoadBalancingInterval"))
        {
            config.MinLoadBalancingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"BalancingDelayAfterNodeDown"))
        {
            config.BalancingDelayAfterNodeDown = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"BalancingDelayAfterNewNode"))
        {
            config.BalancingDelayAfterNewNode = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintFixPartialDelayAfterNodeDown"))
        {
            config.ConstraintFixPartialDelayAfterNodeDown = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintFixPartialDelayAfterNewNode"))
        {
            config.ConstraintFixPartialDelayAfterNewNode = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBRewindInterval"))
        {
            config.PLBRewindInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBActionRetryTimes"))
        {
            config.PLBActionRetryTimes = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxMovementHoldingTime"))
        {
            config.MaxMovementHoldingTime = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxMovementExecutionTime"))
        {
            config.MaxMovementExecutionTime = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AvgStdDevDeltaThrottleThreshold"))
        {
            config.AvgStdDevDeltaThrottleThreshold = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThreshold"))
        {
            config.GlobalMovementThrottleThreshold = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThresholdPercentage"))
        {
            config.GlobalMovementThrottleThresholdPercentage = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThresholdForPlacement"))
        {
            config.GlobalMovementThrottleThresholdForPlacement = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThresholdPercentageForPlacement"))
        {
            config.GlobalMovementThrottleThresholdPercentageForPlacement = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThresholdForBalancing"))
        {
            config.GlobalMovementThrottleThresholdForBalancing = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleThresholdPercentageForBalancing"))
        {
            config.GlobalMovementThrottleThresholdPercentageForBalancing = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMovementThrottleCountingInterval"))
        {
            config.GlobalMovementThrottleCountingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MovementPerPartitionThrottleThreshold"))
        {
            config.MovementPerPartitionThrottleThreshold = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MovementPerPartitionThrottleCountingInterval"))
        {
            config.MovementPerPartitionThrottleCountingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MovementThrottledPartitionsPercentageThreshold"))
        {
            config.MovementThrottledPartitionsPercentageThreshold = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementSearchTimeout"))
        {
            config.PlacementSearchTimeout = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintCheckSearchTimeout"))
        {
            config.ConstraintCheckSearchTimeout = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"FastBalancingSearchTimeout"))
        {
            config.FastBalancingSearchTimeout = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SlowBalancingSearchTimeout"))
        {
            config.SlowBalancingSearchTimeout = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"LoadBalancingEnabled"))
        {
            config.LoadBalancingEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintCheckEnabled"))
        {
            config.ConstraintCheckEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SplitDomainEnabled"))
        {
            config.SplitDomainEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"IsTestMode"))
        {
            config.IsTestMode = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AllowedBalancingScoreDifference"))
        {
            config.AllowedBalancingScoreDifference = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PerfTestNumberOfNodes"))
        {
            config.PerfTestNumberOfNodes = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PerfTestNumberOfPartitions"))
        {
            config.PerfTestNumberOfPartitions = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PerfTestNumberOfInstances"))
        {
            config.PerfTestNumberOfInstances = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PrintRefreshTimers"))
        {
            config.PrintRefreshTimers = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"FaultDomainEnabled"))
        {
            config.FaultDomainEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UpgradeDomainEnabled"))
        {
            config.UpgradeDomainEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"LocalBalancingThreshold"))
        {
            config.LocalBalancingThreshold = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"LocalDomainWeight"))
        {
            config.LocalDomainWeight = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"YieldDurationPer10ms"))
        {
            config.YieldDurationPer10ms = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"InitialRandomSeed"))
        {
            config.InitialRandomSeed = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxSimulatedAnnealingIterations"))
        {
            config.MaxSimulatedAnnealingIterations = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxPercentageToMove"))
        {
            config.MaxPercentageToMove = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"FastBalancingPopulationSize"))
        {
            config.FastBalancingPopulationSize = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SlowBalancingPopulationSize"))
        {
            config.SlowBalancingPopulationSize = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"FastBalancingTemperatureDecayRate"))
        {
            config.FastBalancingTemperatureDecayRate = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SlowBalancingTemperatureDecayRate"))
        {
            config.SlowBalancingTemperatureDecayRate = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"EnableClusterSpecificInitialTemperature"))
        {
            config.EnableClusterSpecificInitialTemperature = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseScoreInConstraintCheck"))
        {
            config.UseScoreInConstraintCheck = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ClusterSpecificTemperatureCoefficient"))
        {
            config.ClusterSpecificTemperatureCoefficient = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"IgnoreCostInScoring"))
        {
            config.IgnoreCostInScoring = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseMoveCostReports"))
        {
            config.UseMoveCostReports = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveCostOffset"))
        {
            config.MoveCostOffset = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveCostZeroValue"))
        {
            config.MoveCostZeroValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveCostLowValue"))
        {
            config.MoveCostLowValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveCostMediumValue"))
        {
            config.MoveCostMediumValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveCostHighValue"))
        {
            config.MoveCostHighValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SwapCost"))
        {
            config.SwapCost = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ScoreImprovementThreshold"))
        {
            config.ScoreImprovementThreshold = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"NodeLoadsTracingInterval"))
        {
            config.NodeLoadsTracingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ApplicationLoadsTracingInterval"))
        {
            config.ApplicationLoadsTracingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBPeriodicalTraceInterval"))
        {
            config.PLBPeriodicalTraceInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxViolatedItemsToTrace"))
        {
            config.MaxViolatedItemsToTrace = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MaxInvalidReplicasToTrace"))
        {
            config.MaxInvalidReplicasToTrace = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MetricActivityThresholds"))
        {
            // MetricActivityThresholds = CpuMin/2,Mem/1
            StringCollection metricActivityThresholdPairs;
            StringUtility::Split<wstring>(it->second, metricActivityThresholdPairs, ItemDelimiter);

            StringMap mapMetricActivityThresholdPairs;
            for (auto metricActivityThresholdPair : metricActivityThresholdPairs)
            {
                StringCollection metricActivityThresholdItems;
                StringUtility::Split<wstring>(metricActivityThresholdPair, metricActivityThresholdItems, PairDelimiter);

                mapMetricActivityThresholdPairs.insert(make_pair(metricActivityThresholdItems[0], metricActivityThresholdItems[1]));
            }

            config.MetricActivityThresholds = Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapMetricActivityThresholdPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MetricBalancingThresholds"))
        {
            // MetricBalancingThresholds = CpuMin/1.2,Mem/1
            StringCollection metricBalancingThresholdPairs;
            StringUtility::Split<wstring>(it->second, metricBalancingThresholdPairs, ItemDelimiter);

            StringMap mapMetricBalancingThresholdPairs;
            for (auto metricBalancingThresholdPair : metricBalancingThresholdPairs)
            {
                StringCollection metricBalancingThresholdItems;
                StringUtility::Split<wstring>(metricBalancingThresholdPair, metricBalancingThresholdItems, PairDelimiter);

                mapMetricBalancingThresholdPairs.insert(make_pair(metricBalancingThresholdItems[0], metricBalancingThresholdItems[1]));
            }

            config.MetricBalancingThresholds = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapMetricBalancingThresholdPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MetricEmptyNodeThresholds"))
        {
            // MetricEmptyNodeThresholds = CpuMin/1.2,Mem/1
            StringCollection metricEmptyNodeThresholdsPairs;
            StringUtility::Split<wstring>(it->second, metricEmptyNodeThresholdsPairs, ItemDelimiter);

            StringMap mapMetricEmptyNodeThresholdsPairs;
            for (auto metricEmptyNodeThresholdsPair : metricEmptyNodeThresholdsPairs)
            {
                StringCollection metricEmptyNodeThresholdsItems;
                StringUtility::Split<wstring>(metricEmptyNodeThresholdsPair, metricEmptyNodeThresholdsItems, PairDelimiter);

                mapMetricEmptyNodeThresholdsPairs.insert(make_pair(metricEmptyNodeThresholdsItems[0], metricEmptyNodeThresholdsItems[1]));
            }

            config.MetricEmptyNodeThresholds = Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapMetricEmptyNodeThresholdsPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"GlobalMetricWeights"))
        {
            // GlobalMetricWeights = CpuMin/1, Mem/0.1
            StringCollection weightMetricPairs;
            StringUtility::Split<wstring>(it->second, weightMetricPairs, ItemDelimiter);

            StringMap mapWeightMetrics;
            for (auto weightMetricPair : weightMetricPairs)
            {
                StringCollection weightMetricItems;
                StringUtility::Split<wstring>(weightMetricPair, weightMetricItems, PairDelimiter);

                mapWeightMetrics.insert(make_pair(weightMetricItems[0], weightMetricItems[1]));
            }

            config.GlobalMetricWeights = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapWeightMetrics);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationMetrics"))
        {
            // DefragmentationMetrics = CpuMin/true, Mem/false
            StringCollection defragMetricPairs;
            StringUtility::Split<wstring>(it->second, defragMetricPairs, ItemDelimiter);

            StringMap mapDefragMetrics;
            for (auto defragMetricPair : defragMetricPairs)
            {
                StringCollection defragMetricItems;
                StringUtility::Split<wstring>(defragMetricPair, defragMetricItems, PairDelimiter);

                mapDefragMetrics.insert(make_pair(defragMetricItems[0], defragMetricItems[1]));
            }

            config.DefragmentationMetrics = Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap::Parse(mapDefragMetrics);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold"))
        {
            // DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold = Cpu/4
            StringCollection defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs;
            StringUtility::Split<wstring>(it->second, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs, ItemDelimiter);

            StringMap mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs;
            for (auto defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPair : defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs)
            {
                StringCollection defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdItems;
                StringUtility::Split<wstring>(defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPair, defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdItems, PairDelimiter);

                mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs.insert(make_pair(defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdItems[0], defragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdItems[1]));
            }

            config.DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapDefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThresholdPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationEmptyNodeDistributionPolicy"))
        {
            // DefragmentationEmptyNodeDistributionPolicy = Cpu/4
            StringCollection defragmentationEmptyNodeDistributionPolicyPairs;
            StringUtility::Split<wstring>(it->second, defragmentationEmptyNodeDistributionPolicyPairs, ItemDelimiter);

            StringMap mapDefragmentationEmptyNodeDistributionPolicyPairs;
            for (auto defragmentationEmptyNodeDistributionPolicyPair : defragmentationEmptyNodeDistributionPolicyPairs)
            {
                StringCollection defragmentationEmptyNodeDistributionPolicyItems;
                StringUtility::Split<wstring>(defragmentationEmptyNodeDistributionPolicyPair, defragmentationEmptyNodeDistributionPolicyItems, PairDelimiter);

                mapDefragmentationEmptyNodeDistributionPolicyPairs.insert(make_pair(defragmentationEmptyNodeDistributionPolicyItems[0], defragmentationEmptyNodeDistributionPolicyItems[1]));
            }

            config.DefragmentationEmptyNodeDistributionPolicy = Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapDefragmentationEmptyNodeDistributionPolicyPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationScopedAlgorithmEnabled"))
        {
            // DefragmentationScopedAlgorithmEnabled = Cpu/true
            StringCollection defragmentationScopedAlgorithmEnabledPairs;
            StringUtility::Split<wstring>(it->second, defragmentationScopedAlgorithmEnabledPairs, ItemDelimiter);

            StringMap mapDefragmentationScopedAlgorithmEnabledPairs;
            for (auto defragmentationScopedAlgorithmEnabledPair : defragmentationScopedAlgorithmEnabledPairs)
            {
                StringCollection defragmentationScopedAlgorithmEnabledItems;
                StringUtility::Split<wstring>(defragmentationScopedAlgorithmEnabledPair, defragmentationScopedAlgorithmEnabledItems, PairDelimiter);

                mapDefragmentationScopedAlgorithmEnabledPairs.insert(make_pair(defragmentationScopedAlgorithmEnabledItems[0], defragmentationScopedAlgorithmEnabledItems[1]));
            }

            config.DefragmentationScopedAlgorithmEnabled = Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap::Parse(mapDefragmentationScopedAlgorithmEnabledPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementStrategy"))
        {
            // PlacementStrategy = CpuMin/1, Mem/0
            StringCollection placementStrategyPairs;
            StringUtility::Split<wstring>(it->second, placementStrategyPairs, ItemDelimiter);

            StringMap mapPlacementStrategy;
            for (auto placementStrategyPair : placementStrategyPairs)
            {
                StringCollection placementStrategyItems;
                StringUtility::Split<wstring>(placementStrategyPair, placementStrategyItems, PairDelimiter);

                mapPlacementStrategy.insert(make_pair(placementStrategyItems[0], placementStrategyItems[1]));
            }

            config.PlacementStrategy = Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap::Parse(mapPlacementStrategy);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RestrictedDefragmentationHeuristicEnabled"))
        {
            config.RestrictedDefragmentationHeuristicEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationEmptyNodeWeight"))
        {
            // DefragmentationEmptyNodeWeight = Cpu/1
            StringCollection defragmentationEmptyNodeWeightPairs;
            StringUtility::Split<wstring>(it->second, defragmentationEmptyNodeWeightPairs, ItemDelimiter);

            StringMap mapDefragmentationEmptyNodeWeightPairs;
            for (auto defragmentationEmptyNodeWeightPair : defragmentationEmptyNodeWeightPairs)
            {
                StringCollection defragmentationEmptyNodeWeightItems;
                StringUtility::Split<wstring>(defragmentationEmptyNodeWeightPair, defragmentationEmptyNodeWeightItems, PairDelimiter);

                mapDefragmentationEmptyNodeWeightPairs.insert(make_pair(defragmentationEmptyNodeWeightItems[0], defragmentationEmptyNodeWeightItems[1]));
            }

            config.DefragmentationEmptyNodeWeight = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapDefragmentationEmptyNodeWeightPairs);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationNodesStdDevFactor"))
        {
            config.DefragmentationNodesStdDevFactor = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationFdsStdDevFactor"))
        {
            config.DefragmentationFdsStdDevFactor = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"DefragmentationUdsStdDevFactor"))
        {
            config.DefragmentationUdsStdDevFactor = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"NodeBufferPercentage"))
        {
            // NodeBufferPercentage CpuMin 0.1 Mem 0.2
            StringCollection nodeBuffers = Utility::Split(it->second, Utility::ParamDelimiter, Utility::EmptyCharacter);
            if (nodeBuffers.size() % 2 == 0)
            {
                StringMap mapNodeBuffPercentage;
                for (int i = 1; i < nodeBuffers.size(); i += 2)
                {
                    mapNodeBuffPercentage.insert(make_pair(nodeBuffers[i], nodeBuffers[i + 1]));
                }
                config.NodeBufferPercentage = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapNodeBuffPercentage);
            }
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PreventIntermediateOvercommit"))
        {
            config.PreventIntermediateOvercommit = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PreventTransientOvercommit"))
        {
            config.PreventTransientOvercommit = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"InBuildThrottlingEnabled"))
        {
            config.InBuildThrottlingEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"InBuildThrottlingGlobalMaxValue"))
        {
            config.InBuildThrottlingGlobalMaxValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SwapPrimaryThrottlingEnabled"))
        {
            config.SwapPrimaryThrottlingEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SwapPrimaryThrottlingGlobalMaxValue"))
        {
            config.SwapPrimaryThrottlingGlobalMaxValue = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"TraceSimulatedAnnealingStatistics"))
        {
            config.TraceSimulatedAnnealingStatistics = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SimulatedAnnealingStatisticsInterval"))
        {
            config.SimulatedAnnealingStatisticsInterval = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"InitialTemperatureProbeCount"))
        {
            config.InitialTemperatureProbeCount = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SimulatedAnnealingIterationsPerRound"))
        {
            config.SimulatedAnnealingIterationsPerRound = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementSearchIterationsPerRound"))
        {
            config.PlacementSearchIterationsPerRound = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ConstraintCheckIterationsPerRound"))
        {
            config.ConstraintCheckIterationsPerRound = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ProcessPendingUpdatesInterval"))
        {
            config.ProcessPendingUpdatesInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"SwapPrimaryProbability"))
        {
            config.SwapPrimaryProbability = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"LoadDecayInterval"))
        {
            config.LoadDecayInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"LoadDecayFactor"))
        {
            config.LoadDecayFactor = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementConstraintPriority"))
        {
            config.PlacementConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PreferredLocationConstraintPriority"))
        {
            config.PreferredLocationConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"CapacityConstraintPriority"))
        {
            config.CapacityConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AffinityConstraintPriority"))
        {
            config.AffinityConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"FaultDomainConstraintPriority"))
        {
            config.FaultDomainConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UpgradeDomainConstraintPriority"))
        {
            config.UpgradeDomainConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ScaleoutCountConstraintPriority"))
        {
            config.ScaleoutCountConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"ApplicationCapacityConstraintPriority"))
        {
            config.ApplicationCapacityConstraintPriority = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"IsAffinityBidirectional"))
        {
            config.IsAffinityBidirectional = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveParentToFixAffinityViolation"))
        {
            config.MoveParentToFixAffinityViolation = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveParentToFixAffinityViolationTransitionPercentage"))
        {
            config.MoveParentToFixAffinityViolationTransitionPercentage = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MoveExistingReplicaForPlacement"))
        {
            config.MoveExistingReplicaForPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseSeparateSecondaryLoad"))
        {
            config.UseSeparateSecondaryLoad = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlaceChildWithoutParent"))
        {
            config.PlaceChildWithoutParent = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AffinityMoveParentReplicaProbability"))
        {
            config.AffinityMoveParentReplicaProbability = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxConstraintsForPlacementEnabled"))
        {
            config.RelaxConstraintsForPlacementEnabled = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxFaultDomainConstraintsForPlacement"))
        {
            config.RelaxFaultDomainConstraintsForPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxAffinityConstraintsForPlacement"))
        {
            config.RelaxAffinityConstraintsForPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxAffinityConstraintDuringUpgrade"))
        {
            config.RelaxAffinityConstraintDuringUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxCapacityConstraintsForPlacement"))
        {
            config.RelaxCapacityConstraintsForPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AllowConstraintCheckFixesDuringApplicationUpgrade"))
        {
            config.AllowConstraintCheckFixesDuringApplicationUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBPeriodicalTraceApplicationCount"))
        {
            config.PLBPeriodicalTraceApplicationCount = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBPeriodicalTraceNodeCount"))
        {
            config.PLBPeriodicalTraceNodeCount = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBPeriodicalTraceServiceCount"))
        {
            config.PLBPeriodicalTraceServiceCount = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBNodeLoadTraceEntryCount"))
        {
            config.PLBNodeLoadTraceEntryCount = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBApplicationLoadTraceMaxSize"))
        {
            config.PLBApplicationLoadTraceMaxSize = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PLBApplicationLoadTraceBatchSize"))
        {
            config.PLBApplicationLoadTraceBatchSize = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PartiallyPlaceServices"))
        {
            config.PartiallyPlaceServices = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"InterruptBalancingForAllFailoverUnitUpdates"))
        {
            config.InterruptBalancingForAllFailoverUnitUpdates = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"CheckAlignedAffinityForUpgrade"))
        {
            config.CheckAlignedAffinityForUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"CheckAffinityForUpgradePlacement"))
        {
            config.CheckAffinityForUpgradePlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxCapacityConstraintForUpgrade"))
        {
            config.RelaxCapacityConstraintForUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxScaleoutConstraintDuringUpgrade"))
        {
            config.RelaxScaleoutConstraintDuringUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxAlignAffinityConstraintDuringSingletonUpgrade"))
        {
            config.RelaxAlignAffinityConstraintDuringSingletonUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseAppGroupsInBoost"))
        {
            config.UseAppGroupsInBoost = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseRGInBoost"))
        {
            config.UseRGInBoost = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"TraceMetricInfoForBalancingRun"))
        {
            config.TraceMetricInfoForBalancingRun = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseDefaultLoadForServiceOnEveryNode"))
        {
            config.UseDefaultLoadForServiceOnEveryNode = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementHeuristicIncomingLoadFactor"))
        {
            // PlacementHeuristicIncomingLoadFactor = CpuMin/1.4, Mem/1.5
            StringCollection placementHeuristicIncomingLoadFactorPairs;
            StringUtility::Split<wstring>(it->second, placementHeuristicIncomingLoadFactorPairs, ItemDelimiter);

            StringMap mapPlacementHeuristicIncomingLoadFactor;
            for (auto placementHeuristicIncomingLoadFactorPair : placementHeuristicIncomingLoadFactorPairs)
            {
                StringCollection placementHeuristicIncomingLoadFactorItems;
                StringUtility::Split<wstring>(placementHeuristicIncomingLoadFactorPair, placementHeuristicIncomingLoadFactorItems, PairDelimiter);

                mapPlacementHeuristicIncomingLoadFactor.insert(make_pair(placementHeuristicIncomingLoadFactorItems[0], placementHeuristicIncomingLoadFactorItems[1]));
            }

            config.PlacementHeuristicIncomingLoadFactor = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapPlacementHeuristicIncomingLoadFactor);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementHeuristicEmptySpacePercent"))
        {
            // PlacementHeuristicIncomingLoadFactor = CpuMin/0.5, Mem/0.2
            StringCollection placementHeuristicEmptySpacePercentPairs;
            StringUtility::Split<wstring>(it->second, placementHeuristicEmptySpacePercentPairs, ItemDelimiter);

            StringMap mapPlacementHeuristicEmptySpacePercent;
            for (auto placementHeuristicEmptySpacePercentPair : placementHeuristicEmptySpacePercentPairs)
            {
                StringCollection placementHeuristicEmptySpacePercentItems;
                StringUtility::Split<wstring>(placementHeuristicEmptySpacePercentPair, placementHeuristicEmptySpacePercentItems, PairDelimiter);

                mapPlacementHeuristicEmptySpacePercent.insert(make_pair(placementHeuristicEmptySpacePercentItems[0], placementHeuristicEmptySpacePercentItems[1]));
            }

            config.PlacementHeuristicEmptySpacePercent = Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap::Parse(mapPlacementHeuristicEmptySpacePercent);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AllowHigherChildTargetReplicaCountForAffinity"))
        {
            config.AllowHigherChildTargetReplicaCountForAffinity = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"QuorumBasedReplicaDistributionPerUpgradeDomains"))
        {
            config.QuorumBasedReplicaDistributionPerUpgradeDomains = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"QuorumBasedReplicaDistributionPerFaultDomains"))
        {
            config.QuorumBasedReplicaDistributionPerFaultDomains = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"QuorumBasedLogicAutoSwitch"))
        {
            config.QuorumBasedLogicAutoSwitch = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AllowBalancingDuringApplicationUpgrade"))
        {
            config.AllowBalancingDuringApplicationUpgrade = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"AutoDetectAvailableResources"))
        {
            config.AutoDetectAvailableResources = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"CpuPercentageNodeCapacity"))
        {
            config.CpuPercentageNodeCapacity = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"MemoryPercentageNodeCapacity"))
        {
            config.MemoryPercentageNodeCapacity = Common::Double_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"EnablePreferredSwapSolutionInConstraintCheck"))
        {
            config.EnablePreferredSwapSolutionInConstraintCheck = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"UseBatchPlacement"))
        {
            config.UseBatchPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"PlacementReplicaCountPerBatch"))
        {
            config.PlacementReplicaCountPerBatch = Common::Int32_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"RelaxConstraintForPlacement"))
        {
            config.RelaxConstraintForPlacement = Bool_Parse(it->second);
        }
        if (StringUtility::AreEqualCaseInsensitive(it->first, L"StatisticsTracingInterval"))
        {
            config.StatisticsTracingInterval = Common::TimeSpan::FromSeconds(Common::Double_Parse(it->second));
        }
    }
}

wstring Utility::GetConfigValue(std::wstring const & configName, Reliability::LoadBalancingComponent::PLBConfig & config)
{
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DummyPLBEnabled"))
    {
        return StringUtility::ToWString(config.DummyPLBEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseCRMPublicName"))
    {
        return StringUtility::ToWString(config.UseCRMPublicName);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"TraceCRMReasons"))
    {
        return StringUtility::ToWString(config.TraceCRMReasons);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ValidatePlacementConstraint"))
    {
        return StringUtility::ToWString(config.ValidatePlacementConstraint);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementConstraintValidationCacheSize"))
    {
        return StringUtility::ToWString(config.PlacementConstraintValidationCacheSize);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintViolationReportingPolicy"))
    {
        return StringUtility::ToWString(config.ConstraintViolationReportingPolicy);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"VerboseHealthReportLimit"))
    {
        return StringUtility::ToWString(config.VerboseHealthReportLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintViolationHealthReportLimit"))
    {
        return StringUtility::ToWString(config.ConstraintViolationHealthReportLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedConstraintViolationHealthReportLimit"))
    {
        return StringUtility::ToWString(config.DetailedConstraintViolationHealthReportLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedVerboseHealthReportLimit"))
    {
        return StringUtility::ToWString(config.DetailedVerboseHealthReportLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConsecutiveDroppedMovementsHealthReportLimit"))
    {
        return StringUtility::ToWString(config.ConsecutiveDroppedMovementsHealthReportLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedNodeListLimit"))
    {
        return StringUtility::ToWString(config.DetailedNodeListLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedPartitionListLimit"))
    {
        return StringUtility::ToWString(config.DetailedPartitionListLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedDiagnosticsInfoListLimit"))
    {
        return StringUtility::ToWString(config.DetailedDiagnosticsInfoListLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DetailedMetricListLimit"))
    {
        return StringUtility::ToWString(config.DetailedMetricListLimit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBHealthEventTTL"))
    {
        return StringUtility::ToWString(config.PLBHealthEventTTL);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBRefreshInterval"))
    {
        return StringUtility::ToWString(config.PLBRefreshInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBRefreshGap"))
    {
        return StringUtility::ToWString(config.PLBRefreshGap);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MinPlacementInterval"))
    {
        return StringUtility::ToWString(config.MinPlacementInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MinConstraintCheckInterval"))
    {
        return StringUtility::ToWString(config.MinConstraintCheckInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MinLoadBalancingInterval"))
    {
        return StringUtility::ToWString(config.MinLoadBalancingInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"BalancingDelayAfterNodeDown"))
    {
        return StringUtility::ToWString(config.BalancingDelayAfterNodeDown);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"BalancingDelayAfterNewNode"))
    {
        return StringUtility::ToWString(config.BalancingDelayAfterNewNode);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintFixPartialDelayAfterNodeDown"))
    {
        return StringUtility::ToWString(config.ConstraintFixPartialDelayAfterNodeDown);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintFixPartialDelayAfterNewNode"))
    {
        return StringUtility::ToWString(config.ConstraintFixPartialDelayAfterNewNode);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBRewindInterval"))
    {
        return StringUtility::ToWString(config.PLBRewindInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxMovementHoldingTime"))
    {
        return StringUtility::ToWString(config.MaxMovementHoldingTime);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxMovementExecutionTime"))
    {
        return StringUtility::ToWString(config.MaxMovementExecutionTime);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AvgStdDevDeltaThrottleThreshold"))
    {
        return StringUtility::ToWString(config.AvgStdDevDeltaThrottleThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThreshold"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThresholdPercentage"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThresholdPercentage);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThresholdForPlacement"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThresholdForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThresholdPercentageForPlacement"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThresholdPercentageForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThresholdForBalancing"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThresholdForBalancing);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleThresholdPercentageForBalancing"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleThresholdPercentageForBalancing);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMovementThrottleCountingInterval"))
    {
        return StringUtility::ToWString(config.GlobalMovementThrottleCountingInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MovementPerPartitionThrottleThreshold"))
    {
        return StringUtility::ToWString(config.MovementPerPartitionThrottleThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MovementPerPartitionThrottleCountingInterval"))
    {
        return StringUtility::ToWString(config.MovementPerPartitionThrottleCountingInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MovementThrottledPartitionsPercentageThreshold"))
    {
        return StringUtility::ToWString(config.MovementThrottledPartitionsPercentageThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementSearchTimeout"))
    {
        return StringUtility::ToWString(config.PlacementSearchTimeout);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintCheckSearchTimeout"))
    {
        return StringUtility::ToWString(config.ConstraintCheckSearchTimeout);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"FastBalancingSearchTimeout"))
    {
        return StringUtility::ToWString(config.FastBalancingSearchTimeout);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SlowBalancingSearchTimeout"))
    {
        return StringUtility::ToWString(config.SlowBalancingSearchTimeout);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"LoadBalancingEnabled"))
    {
        return StringUtility::ToWString(config.LoadBalancingEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintCheckEnabled"))
    {
        return StringUtility::ToWString(config.ConstraintCheckEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SplitDomainEnabled"))
    {
        return StringUtility::ToWString(config.SplitDomainEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"IsTestMode"))
    {
        return StringUtility::ToWString(config.IsTestMode);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AllowedBalancingScoreDifference"))
    {
        return StringUtility::ToWString(config.AllowedBalancingScoreDifference);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PerfTestNumberOfNodes"))
    {
        return StringUtility::ToWString(config.PerfTestNumberOfNodes);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PerfTestNumberOfPartitions"))
    {
        return StringUtility::ToWString(config.PerfTestNumberOfPartitions);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PerfTestNumberOfInstances"))
    {
        return StringUtility::ToWString(config.PerfTestNumberOfInstances);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PrintRefreshTimers"))
    {
        return StringUtility::ToWString(config.PrintRefreshTimers);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"FaultDomainEnabled"))
    {
        return StringUtility::ToWString(config.FaultDomainEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UpgradeDomainEnabled"))
    {
        return StringUtility::ToWString(config.UpgradeDomainEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"LocalBalancingThreshold"))
    {
        return StringUtility::ToWString(config.LocalBalancingThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"LocalDomainWeight"))
    {
        return StringUtility::ToWString(config.LocalDomainWeight);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"YieldDurationPer10ms"))
    {
        return StringUtility::ToWString(config.YieldDurationPer10ms);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InitialRandomSeed"))
    {
        return StringUtility::ToWString(config.InitialRandomSeed);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxSimulatedAnnealingIterations"))
    {
        return StringUtility::ToWString(config.MaxSimulatedAnnealingIterations);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxPercentageToMove"))
    {
        return StringUtility::ToWString(config.MaxPercentageToMove);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxPercentageToMoveForPlacement"))
    {
        return StringUtility::ToWString(config.MaxPercentageToMoveForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"FastBalancingPopulationSize"))
    {
        return StringUtility::ToWString(config.FastBalancingPopulationSize);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SlowBalancingPopulationSize"))
    {
        return StringUtility::ToWString(config.SlowBalancingPopulationSize);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"FastBalancingTemperatureDecayRate"))
    {
        return StringUtility::ToWString(config.FastBalancingTemperatureDecayRate);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SlowBalancingTemperatureDecayRate"))
    {
        return StringUtility::ToWString(config.SlowBalancingTemperatureDecayRate);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"EnableClusterSpecificInitialTemperature"))
    {
        return StringUtility::ToWString(config.EnableClusterSpecificInitialTemperature);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseScoreInConstraintCheck"))
    {
        return StringUtility::ToWString(config.UseScoreInConstraintCheck);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ClusterSpecificTemperatureCoefficient"))
    {
        return StringUtility::ToWString(config.ClusterSpecificTemperatureCoefficient);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"IgnoreCostInScoring"))
    {
        return StringUtility::ToWString(config.IgnoreCostInScoring);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseMoveCostReports"))
    {
        return StringUtility::ToWString(config.UseMoveCostReports);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveCostOffset"))
    {
        return StringUtility::ToWString(config.MoveCostOffset);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveCostZeroValue"))
    {
        return StringUtility::ToWString(config.MoveCostZeroValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveCostLowValue"))
    {
        return StringUtility::ToWString(config.MoveCostLowValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveCostMediumValue"))
    {
        return StringUtility::ToWString(config.MoveCostMediumValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveCostHighValue"))
    {
        return StringUtility::ToWString(config.MoveCostHighValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SwapCost"))
    {
        return StringUtility::ToWString(config.SwapCost);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ScoreImprovementThreshold"))
    {
        return StringUtility::ToWString(config.ScoreImprovementThreshold);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"NodeLoadsTracingInterval"))
    {
        return StringUtility::ToWString(config.NodeLoadsTracingInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ApplicationLoadsTracingInterval"))
    {
        return StringUtility::ToWString(config.ApplicationLoadsTracingInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBPeriodicalTraceInterval"))
    {
        return StringUtility::ToWString(config.PLBPeriodicalTraceInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxViolatedItemsToTrace"))
    {
        return StringUtility::ToWString(config.MaxViolatedItemsToTrace);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MaxInvalidReplicasToTrace"))
    {
        return StringUtility::ToWString(config.MaxInvalidReplicasToTrace);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MetricActivityThresholds"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricActivityThresholds;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MetricBalancingThresholds"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricBalancingThresholds;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MetricEmptyNodeThresholds"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().MetricEmptyNodeThresholds;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"GlobalMetricWeights"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().GlobalMetricWeights;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationMetrics"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetrics;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);

            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationMetricsPercentOrNumberOfEmptyNodesTriggeringThreshold;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationEmptyNodeDistributionPolicy"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationEmptyNodeDistributionPolicy;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationScopedAlgorithmEnabled"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyBoolValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationScopedAlgorithmEnabled;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementStrategy"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyIntegerValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PlacementStrategy;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RestrictedDefragmentationHeuristicEnabled"))
    {
        return StringUtility::ToWString(config.RestrictedDefragmentationHeuristicEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationEmptyNodeWeight"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DefragmentationEmptyNodeWeight;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationNodesStdDevFactor"))
    {
        return StringUtility::ToWString(config.DefragmentationNodesStdDevFactor);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationFdsStdDevFactor"))
    {
        return StringUtility::ToWString(config.DefragmentationFdsStdDevFactor);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"DefragmentationUdsStdDevFactor"))
    {
        return StringUtility::ToWString(config.DefragmentationUdsStdDevFactor);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"NodeBufferPercentage"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().NodeBufferPercentage;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PreventIntermediateOvercommit"))
    {
        return StringUtility::ToWString(config.PreventIntermediateOvercommit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PreventTransientOvercommit"))
    {
        return StringUtility::ToWString(config.PreventTransientOvercommit);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InBuildThrottlingEnabled"))
    {
        return StringUtility::ToWString(config.InBuildThrottlingEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InBuildThrottlingAssociatedMetric"))
    {
        return StringUtility::ToWString(config.InBuildThrottlingAssociatedMetric);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InBuildThrottlingGlobalMaxValue"))
    {
        return StringUtility::ToWString(config.InBuildThrottlingGlobalMaxValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SwapPrimaryThrottlingEnabled"))
    {
        return StringUtility::ToWString(config.SwapPrimaryThrottlingEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SwapPrimaryThrottlingAssociatedMetric"))
    {
        return StringUtility::ToWString(config.SwapPrimaryThrottlingAssociatedMetric);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SwapPrimaryThrottlingGlobalMaxValue"))
    {
        return StringUtility::ToWString(config.SwapPrimaryThrottlingGlobalMaxValue);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"TraceSimulatedAnnealingStatistics"))
    {
        return StringUtility::ToWString(config.TraceSimulatedAnnealingStatistics);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SimulatedAnnealingStatisticsInterval"))
    {
        return StringUtility::ToWString(config.SimulatedAnnealingStatisticsInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InitialTemperatureProbeCount"))
    {
        return StringUtility::ToWString(config.InitialTemperatureProbeCount);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SimulatedAnnealingIterationsPerRound"))
    {
        return StringUtility::ToWString(config.SimulatedAnnealingIterationsPerRound);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementSearchIterationsPerRound"))
    {
        return StringUtility::ToWString(config.PlacementSearchIterationsPerRound);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ConstraintCheckIterationsPerRound"))
    {
        return StringUtility::ToWString(config.ConstraintCheckIterationsPerRound);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ProcessPendingUpdatesInterval"))
    {
        return StringUtility::ToWString(config.ProcessPendingUpdatesInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"SwapPrimaryProbability"))
    {
        return StringUtility::ToWString(config.SwapPrimaryProbability);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"LoadDecayInterval"))
    {
        return StringUtility::ToWString(config.LoadDecayInterval);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"LoadDecayFactor"))
    {
        return StringUtility::ToWString(config.LoadDecayFactor);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementConstraintPriority"))
    {
        return StringUtility::ToWString(config.PlacementConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PreferredLocationConstraintPriority"))
    {
        return StringUtility::ToWString(config.PreferredLocationConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"CapacityConstraintPriority"))
    {
        return StringUtility::ToWString(config.CapacityConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AffinityConstraintPriority"))
    {
        return StringUtility::ToWString(config.AffinityConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"FaultDomainConstraintPriority"))
    {
        return StringUtility::ToWString(config.FaultDomainConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UpgradeDomainConstraintPriority"))
    {
        return StringUtility::ToWString(config.UpgradeDomainConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ScaleoutCountConstraintPriority"))
    {
        return StringUtility::ToWString(config.ScaleoutCountConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"ApplicationCapacityConstraintPriority"))
    {
        return StringUtility::ToWString(config.ApplicationCapacityConstraintPriority);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"IsAffinityBidirectional"))
    {
        return StringUtility::ToWString(config.IsAffinityBidirectional);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveParentToFixAffinityViolation"))
    {
        return StringUtility::ToWString(config.MoveParentToFixAffinityViolation);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveParentToFixAffinityViolationTransitionPercentage"))
    {
        return StringUtility::ToWString(config.MoveParentToFixAffinityViolationTransitionPercentage);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MoveExistingReplicaForPlacement"))
    {
        return StringUtility::ToWString(config.MoveExistingReplicaForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseSeparateSecondaryLoad"))
    {
        return StringUtility::ToWString(config.UseSeparateSecondaryLoad);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlaceChildWithoutParent"))
    {
        return StringUtility::ToWString(config.PlaceChildWithoutParent);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AffinityMoveParentReplicaProbability"))
    {
        return StringUtility::ToWString(config.AffinityMoveParentReplicaProbability);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxConstraintsForPlacementEnabled"))
    {
        return StringUtility::ToWString(config.RelaxConstraintsForPlacementEnabled);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxFaultDomainConstraintsForPlacement"))
    {
        return StringUtility::ToWString(config.RelaxFaultDomainConstraintsForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxAffinityConstraintsForPlacement"))
    {
        return StringUtility::ToWString(config.RelaxAffinityConstraintsForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxAffinityConstraintDuringUpgrade"))
    {
        return StringUtility::ToWString(config.RelaxAffinityConstraintDuringUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxCapacityConstraintsForPlacement"))
    {
        return StringUtility::ToWString(config.RelaxCapacityConstraintsForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AllowConstraintCheckFixesDuringApplicationUpgrade"))
    {
        return StringUtility::ToWString(config.AllowConstraintCheckFixesDuringApplicationUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBPeriodicalTraceApplicationCount"))
    {
        return StringUtility::ToWString(config.PLBPeriodicalTraceApplicationCount);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBPeriodicalTraceNodeCount"))
    {
        return StringUtility::ToWString(config.PLBPeriodicalTraceNodeCount);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBPeriodicalTraceServiceCount"))
    {
        return StringUtility::ToWString(config.PLBPeriodicalTraceServiceCount);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBNodeLoadTraceEntryCount"))
    {
        return StringUtility::ToWString(config.PLBNodeLoadTraceEntryCount);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBApplicationLoadTraceMaxSize"))
    {
        return StringUtility::ToWString(config.PLBApplicationLoadTraceMaxSize);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PLBApplicationLoadTraceBatchSize"))
    {
        return StringUtility::ToWString(config.PLBApplicationLoadTraceBatchSize);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PartiallyPlaceServices"))
    {
        return StringUtility::ToWString(config.PartiallyPlaceServices);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"InterruptBalancingForAllFailoverUnitUpdates"))
    {
        return StringUtility::ToWString(config.InterruptBalancingForAllFailoverUnitUpdates);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"CheckAlignedAffinityForUpgrade"))
    {
        return StringUtility::ToWString(config.CheckAlignedAffinityForUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"CheckAffinityForUpgradePlacement"))
    {
        return StringUtility::ToWString(config.CheckAffinityForUpgradePlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxCapacityConstraintForUpgrade"))
    {
        return StringUtility::ToWString(config.RelaxCapacityConstraintForUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxScaleoutConstraintDuringUpgrade"))
    {
        return StringUtility::ToWString(config.RelaxScaleoutConstraintDuringUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxAlignAffinityConstraintDuringSingletonUpgrade"))
    {
        return StringUtility::ToWString(config.RelaxAlignAffinityConstraintDuringSingletonUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseAppGroupsInBoost"))
    {
        return StringUtility::ToWString(config.UseAppGroupsInBoost);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseRGInBoost"))
    {
        return StringUtility::ToWString(config.UseRGInBoost);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"TraceMetricInfoForBalancingRun"))
    {
        return StringUtility::ToWString(config.TraceMetricInfoForBalancingRun);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseDefaultLoadForServiceOnEveryNode"))
    {
        return StringUtility::ToWString(config.UseDefaultLoadForServiceOnEveryNode);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementHeuristicIncomingLoadFactor"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PlacementHeuristicIncomingLoadFactor;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementHeuristicEmptySpacePercent"))
    {
        wstring result = L"";
        Reliability::LoadBalancingComponent::PLBConfig::KeyDoubleValueMap mapConfig = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().PlacementHeuristicEmptySpacePercent;

        for (auto it = mapConfig.begin(); it != mapConfig.end(); it++)
        {
            wstring metric = StringUtility::ToWString(it->first);
            wstring value = StringUtility::ToWString(it->second);
            result += metric + L"/" + value + L" ";
        }

        return StringUtility::ToWString(result);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AllowHigherChildTargetReplicaCountForAffinity"))
    {
        return StringUtility::ToWString(config.AllowHigherChildTargetReplicaCountForAffinity);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"QuorumBasedReplicaDistributionPerUpgradeDomains"))
    {
        return StringUtility::ToWString(config.QuorumBasedReplicaDistributionPerUpgradeDomains);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"QuorumBasedReplicaDistributionPerFaultDomains"))
    {
        return StringUtility::ToWString(config.QuorumBasedReplicaDistributionPerFaultDomains);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"QuorumBasedLogicAutoSwitch"))
    {
        return StringUtility::ToWString(config.QuorumBasedLogicAutoSwitch);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AllowBalancingDuringApplicationUpgrade"))
    {
        return StringUtility::ToWString(config.AllowBalancingDuringApplicationUpgrade);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"AutoDetectAvailableResources"))
    {
        return StringUtility::ToWString(config.AutoDetectAvailableResources);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"CpuPercentageNodeCapacity"))
    {
        return StringUtility::ToWString(config.CpuPercentageNodeCapacity);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"MemoryPercentageNodeCapacity"))
    {
        return StringUtility::ToWString(config.MemoryPercentageNodeCapacity);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"EnablePreferredSwapSolutionInConstraintCheck"))
    {
        return StringUtility::ToWString(config.EnablePreferredSwapSolutionInConstraintCheck);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"UseBatchPlacement"))
    {
        return StringUtility::ToWString(config.UseBatchPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"PlacementReplicaCountPerBatch"))
    {
        return StringUtility::ToWString(config.PlacementReplicaCountPerBatch);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"RelaxConstraintForPlacement"))
    {
        return StringUtility::ToWString(config.RelaxConstraintForPlacement);
    }
    if (StringUtility::AreEqualCaseInsensitive(configName, L"StatisticsTracingInterval"))
    {
        return StringUtility::ToWString(config.StatisticsTracingInterval);
    }
    return L"Unsupported PLB config...";
}

void Utility::Tokenize(std::wstring & text, std::vector<std::pair<wstring, wstring>> & tokens)
{
    tokens.clear();
    StringCollection tokenVector;
    StringUtility::Trim<wstring>(text, L"{}");
    StringUtility::TrimSpaces(text);
    StringUtility::Split<wstring>(text, tokenVector, VectorDelimiter);

    for (auto it = tokenVector.begin(); it != tokenVector.end(); it++)
    {
        StringUtility::TrimSpaces(*it);
        StringCollection tokenItem;
        StringUtility::Split<wstring>(*it, tokenItem, VectorStartDelimiter);
        ASSERT_IF(tokenItem.size() > 2, "failoverUnit vector {0} has incorrect brace-pairs", tokenVector);
        if (tokenItem.size() == 2)
        {
            StringUtility::TrimSpaces(tokenItem[0]);
            StringUtility::TrimSpaces(tokenItem[1]);
            tokens.push_back(std::make_pair(tokenItem[0], tokenItem[1]));
        }
        else if (it->c_str()[0] != VectorStartDelimiter.c_str()[0])
        {
            tokens.push_back(std::make_pair(tokenItem[0], L""));
        }
        else
        {
            StringUtility::TrimSpaces(tokenItem[0]);
            tokens.push_back(std::make_pair(L"", tokenItem[0]));
        }
    }
}

void Utility::Label(std::vector<std::pair<wstring, wstring>> & tokens,
    std::map<std::wstring, std::wstring> & labelledEntries, std::vector<std::wstring> labels)
{
    //housekeeping..
    labelledEntries.clear();
    if (tokens.size() == 0) { return; }

    //either everything is labelled, or nothing is labelled.
    //If everything is labelled, we simply add stuff to the map.
    //If a particular label has no corresponding entry, we assume the first unlabelled entry belongs to that label. Thus, 'order' of entries is respected in the absence of labels....
    int nextUnlabelledEntry = 0;
    for (int i = 0; i < labels.size(); i++)
    {
        int j;
        for (j = 0; j < tokens.size(); j++)
        {
            if (StringUtility::ContainsCaseInsensitive(tokens[j].first, labels[i]))
            {
                labelledEntries.insert(std::make_pair(labels[i], tokens[j].second));
                break;
            }
        }
        //if we didn't match a label...
        if (j == tokens.size())
        {
            int currentUnlabelledEntry = 0;
            for (j = 0; j < tokens.size(); j++)
            {
                if (tokens[j].first == L"")
                {
                    if (currentUnlabelledEntry == nextUnlabelledEntry)
                    {
                        labelledEntries.insert(std::make_pair(labels[i], tokens[j].second));
                        nextUnlabelledEntry++;
                        break;
                    }
                    else
                    {
                        currentUnlabelledEntry++;
                    }
                }
            }
        }
        //if we didn't have enough unlabelled entries...
        if (j == tokens.size())
        {
            labelledEntries.insert(std::make_pair(labels[i], L""));
        }
    }
}

void Utility::ParseConfigSection(wstring const & fileTextW, size_t & pos, Reliability::LoadBalancingComponent::PLBConfig & config)
{
    size_t oldPos;
    map<wstring, wstring> configMap;

    while (pos != std::wstring::npos)
    {
        oldPos = pos;
        wstring lineBuf = ReadLine(fileTextW, pos);
        if (oldPos == pos || StringUtility::Contains(lineBuf, SectionEnd))
        {
            break;
        }

        StringCollection configEntry;
        StringUtility::Split<wstring>(lineBuf, configEntry, ConfigDelimiter);
        TESTASSERT_IF(configEntry.size() != 2, "config entry has wrong number of key/value pairs", configEntry);

        wstring configKey = configEntry[0];
        wstring configValue = configEntry[1];

        StringUtility::TrimSpaces(configKey);
        StringUtility::TrimSpaces(configValue);
        configMap[configKey] = configValue;
    }

    UpdateConfigValue(configMap, config);
}
void Utility::ReadUint64(const wstring & string, uint64 & integer)
{
    int64 result = 0;
    //we read the last character seperately, so that uint can do the job for us
    wstring partstr;
    partstr.insert(partstr.begin(), string.begin(), string.end() - 1);
    result = _wtoi64(partstr.c_str());
    //we must return 0 if some negative number comes up
    if (result < 0)
    {
        integer = 0;
        return;
    }
    partstr.clear();
    partstr.insert(partstr.begin(), string.end() - 1, string.end());
    integer = uint64(result) * 10;
    integer = integer + _wtoi(partstr.c_str());
}
