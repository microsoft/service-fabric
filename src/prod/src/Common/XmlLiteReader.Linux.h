// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define U_HAVE_INT64_T  1
#define U_HAVE_UINT64_T 1
#include <libxml/xmlreader.h>

namespace Common
{
    class XmlLiteReader;
    typedef std::unique_ptr<XmlLiteReader> XmlLiteReaderUPtr;

    class XmlLiteReader : public IXmlReader, public IUnknown
    {
    public: 
        XmlLiteReader(xmlTextReaderPtr rd) : reader_(rd), refCount_(0) { };

    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

        virtual ULONG STDMETHODCALLTYPE AddRef( void);

        virtual ULONG STDMETHODCALLTYPE Release( void);

    public:
        virtual HRESULT STDMETHODCALLTYPE SetInput( 
            _In_opt_  IUnknown *pInput);
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            _In_  UINT nProperty,
            _Out_  LONG_PTR *ppValue);
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            _In_  UINT nProperty,
            _In_opt_  LONG_PTR pValue);
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            _Out_opt_  XmlNodeType *pNodeType);
        
        virtual HRESULT STDMETHODCALLTYPE GetNodeType( 
            _Out_  XmlNodeType *pNodeType);
        
        virtual HRESULT STDMETHODCALLTYPE MoveToFirstAttribute( void);
        
        virtual HRESULT STDMETHODCALLTYPE MoveToNextAttribute( void);
        
        virtual HRESULT STDMETHODCALLTYPE MoveToAttributeByName( 
            _In_  LPCWSTR pwszLocalName,
            _In_opt_  LPCWSTR pwszNamespaceUri);
        
        virtual HRESULT STDMETHODCALLTYPE MoveToElement( void);
        
        virtual HRESULT STDMETHODCALLTYPE GetQualifiedName( 
            _Outptr_result_buffer_maybenull_(*pcwchQualifiedName+1)  LPCWSTR *ppwszQualifiedName,
            _Out_opt_  UINT *pcwchQualifiedName);
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceUri( 
            _Outptr_result_buffer_maybenull_(*pcwchNamespaceUri+1)  LPCWSTR *ppwszNamespaceUri,
            _Out_opt_  UINT *pcwchNamespaceUri);
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalName( 
            _Outptr_result_buffer_maybenull_(*pcwchLocalName+1)  LPCWSTR *ppwszLocalName,
            _Out_opt_  UINT *pcwchLocalName);
        
        virtual HRESULT STDMETHODCALLTYPE GetPrefix( 
            _Outptr_result_buffer_maybenull_(*pcwchPrefix+1)  LPCWSTR *ppwszPrefix,
            _Out_opt_  UINT *pcwchPrefix);
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            _Outptr_result_buffer_maybenull_(*pcwchValue+1)  LPCWSTR *ppwszValue,
            _Out_opt_  UINT *pcwchValue);
        
        virtual HRESULT STDMETHODCALLTYPE ReadValueChunk( 
            _Out_writes_to_(cwchChunkSize, *pcwchRead)  WCHAR *pwchBuffer,
            _In_  UINT cwchChunkSize,
            _Inout_  UINT *pcwchRead);
        
        virtual HRESULT STDMETHODCALLTYPE GetBaseUri( 
            _Outptr_result_buffer_maybenull_(*pcwchBaseUri+1)  LPCWSTR *ppwszBaseUri,
            _Out_opt_  UINT *pcwchBaseUri);
        
        virtual BOOL STDMETHODCALLTYPE IsDefault( void);
        
        virtual BOOL STDMETHODCALLTYPE IsEmptyElement( void);
        
        virtual HRESULT STDMETHODCALLTYPE GetLineNumber( 
            _Out_  UINT *pnLineNumber);
        
        virtual HRESULT STDMETHODCALLTYPE GetLinePosition( 
            _Out_  UINT *pnLinePosition);
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeCount( 
            _Out_  UINT *pnAttributeCount);
        
        virtual HRESULT STDMETHODCALLTYPE GetDepth( 
            _Out_  UINT *pnDepth);
        
        virtual BOOL STDMETHODCALLTYPE IsEOF( void);

        static HRESULT CreateXmlReader(
            const char *xmlString, 
            _Outptr_ void ** ppvObject);

    private:
        xmlTextReaderPtr reader_;
        vector<wstring> wstrbuf_;
        LONG refCount_;
    };
}

