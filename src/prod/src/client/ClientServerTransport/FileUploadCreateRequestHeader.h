//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Naming
{
    class FileUploadCreateRequestHeader :
        public Transport::MessageHeader<Transport::MessageHeaderId::FileUploadCreateRequest>,
        public Serialization::FabricSerializable
    {
    public:
        FileUploadCreateRequestHeader() { }

        DEFAULT_COPY_CONSTRUCTOR(FileUploadCreateRequestHeader)
        DEFAULT_MOVE_CONSTRUCTOR(FileUploadCreateRequestHeader)

        DEFAULT_COPY_ASSIGNMENT(FileUploadCreateRequestHeader)
        DEFAULT_MOVE_ASSIGNMENT(FileUploadCreateRequestHeader)

        FileUploadCreateRequestHeader(std::wstring const & serviceName, std::wstring const & storeRelativePath, bool const shouldOverwrite, uint64 fileSize);

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() { return serviceName_; }
        
        __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
        inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

        __declspec(property(get=get_ShouldOverwrite)) bool const ShouldOverwrite;
        bool const get_ShouldOverwrite() { return shouldOverwrite_; }

        __declspec(property(get = get_fileSize)) uint64 const FileSize;
        uint64 const get_fileSize() { return fileSize_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_04(serviceName_, storeRelativePath_, shouldOverwrite_, fileSize_);

    private:
        std::wstring serviceName_;
        std::wstring storeRelativePath_;
        bool shouldOverwrite_;
        uint64 fileSize_;
    };
}
