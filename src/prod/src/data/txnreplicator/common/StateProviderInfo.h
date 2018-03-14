// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class StateProviderInfo
    {
    public:
        __declspec(property(get = get_TypeName)) KString::CSPtr TypeName;
        KString::CSPtr StateProviderInfo::get_TypeName() const { return typeName_; }

        __declspec(property(get = get_Name)) KUri::CSPtr Name;
        KUri::CSPtr StateProviderInfo::get_Name() const { return name_; }

        __declspec(property(get = get_InitializationParameters)) Data::Utilities::OperationData::CSPtr InitializationParameters;
        Data::Utilities::OperationData::CSPtr StateProviderInfo::get_InitializationParameters() const { return initializationParameters_; }

    public:
        NOFAIL StateProviderInfo() noexcept;

        NOFAIL StateProviderInfo(
            __in KString const & typeName,
            __in KUri const & name,
            __in_opt Data::Utilities::OperationData const * const initializationParameters) noexcept;

        ~StateProviderInfo();

    private:
        KString::CSPtr typeName_;
        KUri::CSPtr name_;
        Data::Utilities::OperationData::CSPtr initializationParameters_;
    };
}
