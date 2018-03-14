// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Implements IStream interface for reading XML files using XmlLite
    class ComXmlFileStream :
        public IStream,
        private Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
        DENY_COPY(ComXmlFileStream);

        BEGIN_COM_INTERFACE_LIST(ComXmlFileStream)
            COM_INTERFACE_ITEM(IID_IUnknown,IStream)
            COM_INTERFACE_ITEM(IID_IStream,IStream)
            COM_INTERFACE_ITEM(IID_ISequentialStream,ISequentialStream)
        END_COM_INTERFACE_LIST()

    public:
        ComXmlFileStream(HANDLE hFile);
        virtual ~ComXmlFileStream();

        static Common::ErrorCode Open(
            std::wstring fileName, 
            bool fWrite, 
            __out ComPointer<IStream> & xmlFileStream);

    private:
        HANDLE hFile_;

        // ISequentialStream Interface
    public:
        HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead);
        HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten);

        // IStream Interface
    public:
        HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Commit(DWORD) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Revert(void) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE Clone(IStream **) { return E_NOTIMPL; }

        HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer);
        HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag);
    };
}
