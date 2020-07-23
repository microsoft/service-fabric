// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <libxml/xmlreader.h>

class XmlLite : public IXmlReader {
public:
    public XmlLite(wstring const & fileName);

    virtual HRESULT STDMETHODCALLTYPE SetInput(_In_opt_  IUnknown *pInput);
        
    virtual HRESULT STDMETHODCALLTYPE GetProperty(_In_  UINT nProperty, _Out_  LONG_PTR *ppValue);
        
    virtual HRESULT STDMETHODCALLTYPE SetProperty(_In_  UINT nProperty, _In_opt_  LONG_PTR pValue);
        
    virtual HRESULT STDMETHODCALLTYPE Read(_Out_opt_  XmlNodeType *pNodeType);
        
    virtual HRESULT STDMETHODCALLTYPE GetNodeType(_Out_  XmlNodeType *pNodeType);
        
    virtual HRESULT STDMETHODCALLTYPE MoveToFirstAttribute( void);
        
    virtual HRESULT STDMETHODCALLTYPE MoveToNextAttribute( void);
        
    virtual HRESULT STDMETHODCALLTYPE MoveToAttributeByName(_In_  LPCWSTR pwszLocalName, _In_opt_  LPCWSTR pwszNamespaceUri);
        
    virtual HRESULT STDMETHODCALLTYPE MoveToElement( void);
        
    virtual HRESULT STDMETHODCALLTYPE GetQualifiedName(LPCWSTR *ppwszQualifiedName, _Out_opt_  UINT *pcwchQualifiedName);
        
    virtual HRESULT STDMETHODCALLTYPE GetNamespaceUri(LPCWSTR *ppwszNamespaceUri, _Out_opt_  UINT *pcwchNamespaceUri);
        
    virtual HRESULT STDMETHODCALLTYPE GetLocalName(LPCWSTR *ppwszLocalName, _Out_opt_  UINT *pcwchLocalName);
        
    virtual HRESULT STDMETHODCALLTYPE GetPrefix(LPCWSTR *ppwszPrefix, _Out_opt_  UINT *pcwchPrefix);
        
    virtual HRESULT STDMETHODCALLTYPE GetValue(LPCWSTR *ppwszValue, _Out_opt_  UINT *pcwchValue);
        
    virtual HRESULT STDMETHODCALLTYPE ReadValueChunk(WCHAR *pwchBuffer, _In_  UINT cwchChunkSize, _Inout_  UINT *pcwchRead);
        
    virtual HRESULT STDMETHODCALLTYPE GetBaseUri(LPCWSTR *ppwszBaseUri, _Out_opt_  UINT *pcwchBaseUri);
        
    virtual BOOL STDMETHODCALLTYPE IsDefault( void);
        
    virtual BOOL STDMETHODCALLTYPE IsEmptyElement( void);
        
    virtual HRESULT STDMETHODCALLTYPE GetLineNumber(_Out_  UINT *pnLineNumber);
        
    virtual HRESULT STDMETHODCALLTYPE GetLinePosition(_Out_  UINT *pnLinePosition);
        
    virtual HRESULT STDMETHODCALLTYPE GetAttributeCount(_Out_  UINT *pnAttributeCount);
        
    virtual HRESULT STDMETHODCALLTYPE GetDepth(_Out_  UINT *pnDepth);
        
    virtual BOOL STDMETHODCALLTYPE IsEOF( void);

private:
    xmlTextReaderPtr reader_;
};

