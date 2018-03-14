// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class FileSequenceHeader : 
        public Transport::MessageHeader<Transport::MessageHeaderId::FileSequence>, 
        public Serialization::FabricSerializable
    {
    public:
        FileSequenceHeader() 
            : sequenceNumber_(0)
            , isLast_(false)
            , bufferSize_(0)
        { 
        }

        explicit FileSequenceHeader(uint64 const sequenceNumber, bool const isLast, uint64 const bufferSize) 
            : sequenceNumber_(sequenceNumber)
            , isLast_(isLast)
            , bufferSize_(bufferSize)
        {
        }

        FileSequenceHeader(FileSequenceHeader const & other) 
            : sequenceNumber_(other.sequenceNumber_)
            , isLast_(other.isLast_)
            , bufferSize_(other.bufferSize_)
        {
        }

        __declspec(property(get=get_SequenceNumber)) uint64 const SequenceNumber;
        inline uint64 const get_SequenceNumber() const { return sequenceNumber_; }        

        __declspec(property(get=get_IsLast)) bool const IsLast;
        bool const get_IsLast() { return isLast_; }

        __declspec(property(get=get_BufferSize)) uint64 const BufferSize;
        uint64 const get_BufferSize() { return bufferSize_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        { 
            w.Write("FileSequenceHeader{ ");
            w.Write("SequenceNumber = {0},", sequenceNumber_);
            w.Write("IsLast = {0},", isLast_);
            w.Write("BufferSize = {0},", bufferSize_);
            w.Write(" }");
        }

        FABRIC_FIELDS_03(sequenceNumber_, isLast_, bufferSize_);

    private:
        uint64 sequenceNumber_;
        bool isLast_;
        uint64 bufferSize_;
    };
}
