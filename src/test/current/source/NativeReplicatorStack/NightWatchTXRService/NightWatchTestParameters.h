// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    class NightWatchTestParameters
        : public Common::IFabricJsonSerializable
    {
    public:

        NightWatchTestParameters()
            : numberOfOperations_((ULONG)0)
            , maxOutstandingOperations_((ULONG)0)
            , valueSizeInBytes_((ULONG)0)
            , action_(TestAction::Enum::Invalid)
            , updateOperationValue_(L"")
        {
        }

        NightWatchTestParameters(
            __in ULONG numberOfOperations,
            __in ULONG maxOutstandingOperations,
            __in ULONG valueSizeInBytes,
            __in TestAction::Enum const & action)
            : numberOfOperations_(numberOfOperations)
            , maxOutstandingOperations_(maxOutstandingOperations)
            , valueSizeInBytes_(valueSizeInBytes)
            , action_(action)
        {
            PopulateUpdateOperationValue();
        }

        __declspec(property(get = get_numberOfOperations)) ULONG NumberOfOperations;
        ULONG get_numberOfOperations() const
        {
            return numberOfOperations_;
        }

        __declspec(property(get = get_maxOutstandingOperations)) ULONG MaxOutstandingOperations;
        ULONG get_maxOutstandingOperations() const
        {
            return maxOutstandingOperations_;
        }

        __declspec(property(get = get_valueSizeInBytes)) ULONG ValueSizeInBytes;
        ULONG get_valueSizeInBytes() const
        {
            return valueSizeInBytes_;
        }

        _declspec(property(get = get_action)) TestAction::Enum Action;
        TestAction::Enum get_action() const
        {
            return action_;
        }

        _declspec(property(get = get_updateOperationValue)) std::wstring UpdateOperationValue;
        std::wstring get_updateOperationValue() const
        {
            return updateOperationValue_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "NumberOfOperations = '{0}', MaxOutstandingOperations = '{1}', ValueSizeInBytes = '{2}', Action='{3}'",
                numberOfOperations_,
                maxOutstandingOperations_,
                valueSizeInBytes_,
                TestAction::ToString(action_));
        }

        void PopulateUpdateOperationValue()
        {
            updateOperationValue_ = L"";
            
            // Since 1 wchar_t is 2 bytes, divide operation size by 2
            // Additionally test state provider adds redo and undo of this size. Hence divide by 2 again
            for (ULONG i = 0; i < (valueSizeInBytes_ / 4); i++)
            {
                updateOperationValue_ = updateOperationValue_ + L"a";
            }
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"NumberOfOperations", numberOfOperations_)
            SERIALIZABLE_PROPERTY(L"MaxOutstandingOperations", maxOutstandingOperations_)
            SERIALIZABLE_PROPERTY(L"ValueSizeInBytes", valueSizeInBytes_)
            SERIALIZABLE_PROPERTY_ENUM(L"Action", action_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        ULONG numberOfOperations_;
        ULONG maxOutstandingOperations_;
        ULONG valueSizeInBytes_;
        TestAction::Enum action_;

        std::wstring updateOperationValue_;
    };
}
