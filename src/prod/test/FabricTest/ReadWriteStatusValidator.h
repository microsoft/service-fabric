// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ReadWriteStatusValidator
    {
    public:
        explicit ReadWriteStatusValidator(Common::ComPointer<IFabricStatefulServicePartition> const& partition);

        void OnOpen();
        void OnClose();
        void OnAbort();
        void OnChangeRole(FABRIC_REPLICA_ROLE newRole);

        void OnOnDataLoss();

    private:
        class StatusType
        {
        public:
            enum Enum
            {
                Read,
                Write
            };
        };

        void ValidateStatusIsNotPrimary(
            std::wstring const & tag);

        void ValidateStatusIsTryAgain(
            std::wstring const & tag);

        void ValidateStatusIsNotPrimaryOrTryAgain(
            std::wstring const & tag);

        void ValidateStatusIs(
            std::wstring const & tag,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected1);

        void ValidateStatusIs(
            std::wstring const & tag,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected1,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected2);

        void ValidateStatusIs(
            StatusType::Enum type,
            std::wstring const & tag,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS * e1,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS * e2);

        bool TryGetStatus(
            StatusType::Enum type,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS & status);

        Common::ComPointer<IFabricStatefulServicePartition> partition_;
    };

    typedef std::unique_ptr<ReadWriteStatusValidator> ReadWriteStatusValidatorUPtr;
};
