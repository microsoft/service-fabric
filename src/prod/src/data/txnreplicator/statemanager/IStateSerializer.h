// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        template <typename T>
        interface IStateSerializer
        {
            K_SHARED_INTERFACE(IStateSerializer)

        public:

            //
            // Deserializes from the given BinaryReader T
            // to deserialize from
            // When accessing the Binarywriter base stream, care must be taken when moving the position in the stream.
            // Reading must begin at the current stream position and end at the current position plus the length of your data.
            // 
            virtual T Read(__in Data::Utilities::BinaryReader& binaryReader) = 0;

            //
            // Serializes a value and writes it to the given binary writer
            // When accessing the Binary writer base stream, care must be taken when moving the position in the stream.
            // Writing must begin at the current stream position and end at the current position plus the length of your data.
            // 
            virtual void Write(
                __in T value, 
                __in Data::Utilities::BinaryWriter& binaryWriter) = 0;

            // 
            // Deserializes from the given Binary writer to T
            // When accessing the BinaryReader base stream, care must be taken when moving the position in the stream.
            // Reading must begin at the current stream position and end at the current position plus the length of your data.
            //
            //virtual T Read(
            //    __in T& baseValue, 
            //    __in Data::Utilities::BinaryReader& binaryReader) = 0;

            //
            // Serializes an object and writes it to the given Binary writer
            // When accessing the BinaryWriter base stream, care must be taken when moving the position in the stream.
            // Writing must begin at the current stream position and end at the current position plus the length of your data.
            // 
            /*virtual void Write(
                __in T& baseValue, 
                __in T& targetValue, 
                __in Data::Utilities::BinaryWriter& binaryWriter) = 0;*/
        };
    }
}
