// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    class FabricStream : public IFabricStream, public KObject<FabricStream>
    {
    public:
        typedef KUniquePtr<IFabricStream, IKBaseDeleter<IFabricStream>> UPtr;

        static NTSTATUS Create(__out IFabricStream ** outPtr, ULONG maxGrowBy = 0);

        static NTSTATUS Create(__out IFabricStream ** outPtr, __in KMemChannel::UPtr && byteStream, __in KArray<FabricIOBuffer> && extensionBuffers);

        virtual NTSTATUS QueryInterface(
            __in  GUID&  InterfaceId,
            __out VOID **Interface
            );

        virtual LONG AddRef();

        virtual LONG Release();

        virtual NTSTATUS WriteBytes(__in ULONG size, __in VOID const * source);

        virtual NTSTATUS WriteBytesNoCopy(__in ULONG size, __in VOID const * source);

        virtual NTSTATUS ReadBytes(__in ULONG size, __out VOID * destination);

        virtual NTSTATUS ReadBytesNoCopy(__in ULONG index, __out FabricIOBuffer & buffer);

        NTSTATUS GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData) override;

        NTSTATUS GetAllBuffers(_Out_ FabricIOBuffer* bufferArray, _Inout_ size_t & count) const override;

        virtual NTSTATUS Seek(__in ULONG position);

        virtual NTSTATUS SeekToEnd();

        virtual ULONG get_Position() const;

        ULONG Size() const override;

        virtual NTSTATUS AddCompletionCallback(__in FabricCompletionCallback completionCallback, __in VOID * state);

        virtual NTSTATUS InvokeCompletionCallbacks();

        ~FabricStream();

    private:

        FAILABLE FabricStream(ULONG maxGrowBy = 0);

        FAILABLE FabricStream(__in KMemChannel::UPtr && byteStream, __in KArray<FabricIOBuffer> && extensionBuffers);

        KMemChannel::UPtr byteStream_;
        ULONG regularDataCursor_;
        ULONG extensionDataCursor_;
        LONG RefCount_;
        bool cursorOnExtensionData_;

        struct _CallbackInfo
        {
            FabricCompletionCallback callback_;
            VOID * state_;

            void Invoke()
            {
                this->callback_(this->state_);
            }
        };

        KArray<FabricIOBuffer> extensionBuffers_;
        KArray<_CallbackInfo> completionCallbacks_; // TODO: use a delegate
    };
}
