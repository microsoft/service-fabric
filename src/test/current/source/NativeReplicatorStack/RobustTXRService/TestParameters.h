// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace RobustTXRService
{
    class TestParameters
        : public Common::IFabricJsonSerializable
    {
    public:

        TestParameters()
            : workloadType_(L"")
            , numberOfTransactions_((ULONG)0)
            , minDataPerTransactionInKb_((ULONG)0)
            , maxDataPerTransactionInKb_((ULONG)0)
            , skipTransactionCompletionRatio_((ULONG)0)
            , action_(NightWatchTXRService::TestAction::Enum::Invalid)
        {
        }

        TestParameters(
            __in std::wstring workloadType,
            __in ULONG numberOfTransactions,
            __in ULONG minDataPerTransactionInKb,
            __in ULONG maxDataPerTransactionInKb,
            __in ULONG skipTransactionCompletionRatio,
            __in NightWatchTXRService::TestAction::Enum action)
            : workloadType_(workloadType)
            , numberOfTransactions_(numberOfTransactions)
            , minDataPerTransactionInKb_(minDataPerTransactionInKb)
            , maxDataPerTransactionInKb_(maxDataPerTransactionInKb)
            , skipTransactionCompletionRatio_(skipTransactionCompletionRatio)
            , action_(action)
        {
        }

        _declspec(property(get = get_workloadType)) std::wstring WorkloadType;
        std::wstring get_workloadType() const
        {
            return workloadType_;
        }

        __declspec(property(get = get_numberOfTransactions)) ULONG NumberOfTransactions;
        ULONG get_numberOfTransactions() const
        {
            return numberOfTransactions_;
        }

        __declspec(property(get = get_minDataPerTransactionInKb)) ULONG MinDataPerTransactionInKb;
        ULONG get_minDataPerTransaction() const
        {
            return minDataPerTransactionInKb_;
        }

        __declspec(property(get = get_maxDataPerTransactionInKb)) ULONG MaxDataPerTransactionInKb;
        ULONG get_maxDataPerTransaction() const
        {
            return maxDataPerTransactionInKb_;
        }

        __declspec(property(get = get_skipTransactionCompletionRatio)) double SkipTransactionCompletionRatio;
        double get_skipTransactionCompletionRatio() const
        {
            return skipTransactionCompletionRatio_;
        }

        _declspec(property(get = get_action)) NightWatchTXRService::TestAction::Enum Action;
        NightWatchTXRService::TestAction::Enum get_action() const
        {
            return action_;
        }

        _declspec(property(get = get_operationValuesCount)) ULONG OperationValuesCount;
        ULONG get_operationValuesCount() const
        {
            return (ULONG)operationValues_.size();
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "WorkloadType = '{0}', NumberOfTransactions = '{1}', MinDataPerTransatcionInKb = '{2}', MaxDataPerTransactionInKb = '{3}', SkipTransactionCompletionRatio = '{4}', Action = '{5}'",
                workloadType_,
                numberOfTransactions_,
                minDataPerTransactionInKb_,
                maxDataPerTransactionInKb_,
                skipTransactionCompletionRatio_,
                action_);
        }

        void PopulateUpdateOperationValues()
        {
            //creating different operation values (200K bytes differences) to randomly pick from.
            for (ULONG size = minDataPerTransactionInKb_; size < maxDataPerTransactionInKb_; size += 200)
            {
                ULONG sizeInBytes = size * 1024;
                std::wstring val = L"";
                // Since 1 wchar_t is 2 bytes, divide operation size by 2
                // Additionally test state provider adds redo and undo of this size. Hence divide by 2 again
                for (ULONG i = 0; i < (sizeInBytes / 4); i++)
                {
                    val += L"a";
                }
                operationValues_.push_back(val);
            }
        }

        std::wstring GetOperationValue(ULONG randIndex)
        {
            if (randIndex >= operationValues_.size())
            {
                return L"";
            }
            return operationValues_[randIndex];
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"WorkloadType", workloadType_)
            SERIALIZABLE_PROPERTY(L"NumberOfTransactions", numberOfTransactions_)
            SERIALIZABLE_PROPERTY(L"MinDataPerTransactionInKb", minDataPerTransactionInKb_)
            SERIALIZABLE_PROPERTY(L"MaxDataPerTransactionInKb", maxDataPerTransactionInKb_)
            SERIALIZABLE_PROPERTY(L"SkipTransactionCompletionRatio", skipTransactionCompletionRatio_)
            SERIALIZABLE_PROPERTY_ENUM(L"Action", action_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring workloadType_;
        ULONG numberOfTransactions_;
        ULONG minDataPerTransactionInKb_;
        ULONG maxDataPerTransactionInKb_;
        double skipTransactionCompletionRatio_;
        NightWatchTXRService::TestAction::Enum action_;

        std::vector<std::wstring> operationValues_;
    };
}