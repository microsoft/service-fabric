// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class FileUploadRequestHeader : 
        public Transport::MessageHeader<Transport::MessageHeaderId::FileUploadRequest>, 
        public Serialization::FabricSerializable
    {
    public:
        FileUploadRequestHeader() { }

        explicit FileUploadRequestHeader(std::wstring const & serviceName, std::wstring const & storeRelativePath, bool const shouldOverwrite) 
            : serviceName_(serviceName)
            , storeRelativePath_(storeRelativePath)
            , shouldOverwrite_(shouldOverwrite)
        {
        }

        FileUploadRequestHeader(FileUploadRequestHeader const & other) 
            : serviceName_(other.serviceName_)
            , storeRelativePath_(other.storeRelativePath_)    
            , shouldOverwrite_(other.shouldOverwrite_)
        {
        }

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() { return serviceName_; }
        
        __declspec(property(get=get_StoreRelativePath)) std::wstring const & StoreRelativePath;
        inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }        

        __declspec(property(get=get_ShouldOverwrite)) bool const ShouldOverwrite;
        bool const get_ShouldOverwrite() { return shouldOverwrite_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("FileMetadataHeader{ ");
            w.Write("ServiceName = {0},", serviceName_);
            w.Write("StoreRelativePath = {0},", storeRelativePath_);
            w.Write("ShouldOvewrite = {0},", shouldOverwrite_);
            w.Write(" }");
        }

        FABRIC_FIELDS_03(serviceName_, storeRelativePath_, shouldOverwrite_);

    private:
        std::wstring serviceName_;
        std::wstring storeRelativePath_;
        bool shouldOverwrite_;
    };
}
