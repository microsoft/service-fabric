// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class FileStreamCopyOperationData : public Serialization::FabricSerializable
    {
    public:
        FileStreamCopyOperationData()
            : isFirstChunk_(false)
            , data_()
            , isLastChunk_(false)
        {
        }

        FileStreamCopyOperationData(
            bool isFirstChunk,
            std::vector<byte> && data,
            bool isLastChunk)
            : isFirstChunk_(isFirstChunk)
            , data_(std::move(data))
            , isLastChunk_(isLastChunk)
        {
        }

        __declspec(property(get=get_IsFirstChunk)) bool IsFirstChunk;
        bool get_IsFirstChunk() const { return isFirstChunk_; }

        __declspec(property(get=get_IsLastChunk)) bool IsLastChunk;
        bool get_IsLastChunk() const { return isLastChunk_; }

        Common::ErrorCode TakeData(__out std::vector<byte> & data)
        {
            data = std::move(data_);
            return Common::ErrorCodeValue::Success;
        }

        FABRIC_FIELDS_03(isFirstChunk_, data_, isLastChunk_);

    private:
        bool isFirstChunk_;
        std::vector<byte> data_;
        bool isLastChunk_;
    };
}
