// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Wraps the read and write status
        class ReadWriteStatusValue
        {
        public:
            ReadWriteStatusValue() :
                readStatus_(AccessStatus::NotPrimary),
                writeStatus_(AccessStatus::NotPrimary)
            {
            }

            ReadWriteStatusValue(AccessStatus::Enum readStatus, AccessStatus::Enum writeStatus) :
                readStatus_(readStatus),
                writeStatus_(writeStatus)
            {
            }

            ReadWriteStatusValue(ReadWriteStatusValue const & other) :
                readStatus_(other.readStatus_),
                writeStatus_(other.writeStatus_)
            {
            }

            ReadWriteStatusValue(ReadWriteStatusValue && other) :
                readStatus_(std::move(other.readStatus_)),
                writeStatus_(std::move(other.writeStatus_))
            {
            }

            ReadWriteStatusValue & operator=(ReadWriteStatusValue&& other)
            {
                if (this != &other)
                {
                    readStatus_ = std::move(other.readStatus_);
                    writeStatus_ = std::move(other.writeStatus_);
                }

                return *this;
            }

            ReadWriteStatusValue & operator=(ReadWriteStatusValue const & other)
            {
                if (this != &other)
                {
                    readStatus_ = other.readStatus_;
                    writeStatus_ = other.writeStatus_;
                }

                return *this;
            }

            Common::ErrorCode TryGet(
                AccessStatusType::Enum type,
                __out AccessStatus::Enum & value) const
            {
                if (type == AccessStatusType::Read)
                {
                    value = readStatus_;
                }
                else
                {
                    value = writeStatus_;
                }

                return Common::ErrorCode::Success();
            }

        private:
            AccessStatus::Enum readStatus_;
            AccessStatus::Enum writeStatus_;
        };
    }
}
