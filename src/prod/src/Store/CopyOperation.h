// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class CopyOperation : public Serialization::FabricSerializable
    {
        CopyOperation & operator=(CopyOperation const &) = delete;

    public:
        CopyOperation() 
            : isFirstFullCopy_(false)
            , operations_()
            , copyType_()
            , fileStreamCopyOperationDataUPtr_()
        { 
        }

        CopyOperation(
            Store::CopyType::Enum copyType,
            std::vector<ReplicationOperation> && operations)
            : isFirstFullCopy_(copyType == CopyType::FirstFullCopy)
            , operations_(std::move(operations))
            , copyType_(copyType)
            , fileStreamCopyOperationDataUPtr_()
        {
        }

        CopyOperation(
            std::unique_ptr<FileStreamCopyOperationData> && fileStreamData,
            bool isRebuildMode)
            : isFirstFullCopy_(false)
            , operations_()
            , copyType_(isRebuildMode ? CopyType::FileStreamRebuildCopy : CopyType::FileStreamFullCopy)
            , fileStreamCopyOperationDataUPtr_(std::move(fileStreamData))
        {
        }

        CopyOperation(
            CopyOperation && other)
            : isFirstFullCopy_(std::move(other.isFirstFullCopy_))
            , operations_(std::move(other.operations_))
            , copyType_(std::move(other.copyType_))
            , fileStreamCopyOperationDataUPtr_(std::move(other.fileStreamCopyOperationDataUPtr_))
        {
        }

        CopyOperation & operator=(CopyOperation && other)
        {
            if (this != &other)
            {
                isFirstFullCopy_ = std::move(other.isFirstFullCopy_);
                operations_ = std::move(other.operations_);
                copyType_ = std::move(other.copyType_);
                fileStreamCopyOperationDataUPtr_ = std::move(other.fileStreamCopyOperationDataUPtr_);
            }

            return *this;
        }

        __declspec(property(get=get_IsFirstFullCopy)) bool IsFirstFullCopy;
        bool get_IsFirstFullCopy() const { return isFirstFullCopy_; }

        __declspec(property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const 
        { 
            return (copyType_ != CopyType::FileStreamFullCopy
                && copyType_ != CopyType::FileStreamRebuildCopy
                && operations_.empty()); 
        }

        __declspec(property(get=get_CopyType)) Store::CopyType::Enum CopyType;
        Store::CopyType::Enum get_CopyType() const 
        { 
            // Backwards compatibility with versions that only sent
            // isFirstFullCopy_ flag (no CopyType enum).
            //
            return (isFirstFullCopy_ ? CopyType::FirstFullCopy : copyType_);
        }

        __declspec(property(get=get_FileStreamCopyOperationData)) std::unique_ptr<FileStreamCopyOperationData> const & FileStreamData;
        std::unique_ptr<FileStreamCopyOperationData> const & get_FileStreamCopyOperationData() const { return fileStreamCopyOperationDataUPtr_; }

        Common::ErrorCode TakeReplicationOperations(__out std::vector<ReplicationOperation> & operations)
        {
            operations = move(operations_);
            return Common::ErrorCodeValue::Success;
        }

        FABRIC_FIELDS_04(isFirstFullCopy_, operations_, copyType_, fileStreamCopyOperationDataUPtr_);

    private:
        bool isFirstFullCopy_; // obsolete field - preserved for backwards compatibility
        std::vector<ReplicationOperation> operations_;
        Store::CopyType::Enum copyType_;
        std::unique_ptr<FileStreamCopyOperationData> fileStreamCopyOperationDataUPtr_;
    };
}
