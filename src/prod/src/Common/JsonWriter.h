// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This constant defines the maximum nesting depth that the parser supports. The JSON spec states that this
// is an implementation dependent thing, so we're just picking a value for now. FWIW .Net chose 100
//
// Note: This value needs to be a multiple of 8 and must be less than 2^15
//
#ifndef MAX_STACK_SIZE
#define MAX_STACK_SIZE 128
#endif

#define JSON_E_MAX_NESTING_EXCEEDED                                 _HRESULT_TYPEDEF_(0x803F0010)
#define JSON_E_MISSING_PROPERTY                                     _HRESULT_TYPEDEF_(0x80380001)
#define JSON_E_PROPERTY_ALREADY_ADDED                               _HRESULT_TYPEDEF_(0x80380002)
#define JSON_E_OBJECT_NOT_STARTED                                   _HRESULT_TYPEDEF_(0x80380003)
#define JSON_E_ARRAY_NOT_STARTED                                    _HRESULT_TYPEDEF_(0x80380004)
#define JSON_E_PROPERTY_NOT_REQUIRED                                _HRESULT_TYPEDEF_(0x80380005)
#define JSON_E_PROPERTY_ARRAY_OR_OBJECT_NOT_STARTED                 _HRESULT_TYPEDEF_(0x80380006)
#define JSON_E_NOT_COMPLETE                                         _HRESULT_TYPEDEF_(0x80380007)

#define ValueSeparatorToken     (':')
#define MemberSeparatorToken    (',')
#define ObjectStartToken        ('{')
#define ObjectEndToken          ('}')
#define ArrayStartToken         ('[')
#define ArrayEndToken           (']')
#define PropertyStartToken      ('"')
#define PropertyEndToken        ('"')
#define StringStartToken        ('"')
#define StringEndToken          ('"')

namespace Common
{
    //-------------------------------------------------------------------------------------------------------------------------------------------
    // Note: This class is NOT a generalized stack of WCHARs. It ONLY supports '{' and '['
    class JSONStack
    {
    public:
        JSONStack(): m_nIndex(-1)
        {
            memset(m_values, 0, _countof(m_values));
        }

        ~JSONStack()
        {
        }

        inline bool IsEmpty() const
        {
            return m_nIndex == -1;
        }
        // peek will never fail because it returns char if it finds it, but it returns 0 if it does not find it
        //
        inline WCHAR Peek() const
        {
            return IsEmpty() ? 0 : GetChar();
        }

        HRESULT Pop(__in_ecount(1) WCHAR* pChar)
        {
            if(!pChar) return E_POINTER;
            if(IsEmpty()) return E_FAIL;
            WCHAR result = GetChar();
            m_nIndex--;
            *pChar = result;
            return S_OK;
        }

        HRESULT Push(WCHAR value)
        {
            if(m_nIndex + 1 >= MAX_STACK_SIZE || (value != '{' && value != '[')) return E_FAIL;
            m_nIndex++;

            if(value == '[')
            {
                m_values[m_nIndex / 8] &= ~GetMask();
            }
            else
            {
                m_values[m_nIndex / 8] |= GetMask();
            }

            return S_OK;
        }

        ULONG GetDepth() const
        {
            if (IsEmpty())
                return 0;
            else
                return static_cast<ULONG>(m_nIndex) + 1;
        }

    private:
        BYTE m_values[MAX_STACK_SIZE / 8];
        SHORT m_nIndex;

        // Note: In GetChar we take the current index and divide by 8 (the number of bits in a byte)
        // to get the index in m_values of the byte that contains the bit that is at the top of the stack.
        // In GetMask, we use modulos division by 8 to get the index of the bit at the top of the stack
        // relative to the byte we found by doing the integer division.
        //
        // e.g. If the stack currently contains 9 items, then 9 / 8 = 1, so we need the m_values[1].
        //      9 % 8 = 1, so we shift 1 left by 1 place and we get the 9th bit.
        //
        //      If the stack currently contains 5 items, then 5 / 8 = 0, so we need the m_values[0].
        //      5 % 8 = 5, so we shift 1 left by 5 places and we get the 5th bit.
        //
        inline WCHAR GetChar() const
        {
            return (m_values[m_nIndex / 8] & GetMask()) == 0 ? '[' : '{';
        }
        inline BYTE GetMask() const
        {
            return 1 << (m_nIndex  % 8);
        }
    };
    //-------------------------------------------------------------------------------------------------------------------------------------------
    // This class is used to build a json string. It supports our defined IJsonWriter interface.
    // It uses cstring buffer for building json string. It creates only one instance of cstring to reduce any avoidable overhead.
    // It keeps an stack to keep track of scope, and provides error checking using that. It has few other variables for error checking.
    // user can also provide initial size to reserve string buffer, that will help reduce cost of reallocation.
    // it provides error checking based on json grammer. It provides escaping for nine characters specified in json.
    //

    class  JsonWriter: public IJsonWriter, private Common::ComUnknownBase
    {

    public:
        JsonWriter():m_bFirstValue(true), m_bPropertyAppended(false)
        {
        }

        ~JsonWriter()
        {
        }

    public:
        BEGIN_COM_INTERFACE_LIST(JsonWriter)
            COM_INTERFACE_ITEM(__uuidof(IJsonWriter), IJsonWriter)
            END_COM_INTERFACE_LIST()

    public:

        HRESULT GetBytes(std::vector<BYTE> &bytes)
        {
            if(IsComplete())
            {
                bytes.resize(m_strBuffer.length());
                memcpy_s(bytes.data(), m_strBuffer.length(), m_strBuffer.c_str(), m_strBuffer.length());
                return S_OK;
            }
            else
            {
                return JSON_E_NOT_COMPLETE;
            }
        }

    public:
        //IJsonWriter
        STDMETHODIMP PropertyName(LPCWSTR pszStr)
        {
            if(!pszStr) return E_POINTER;

            // property should not be added inside array
            //
            if(m_stack.Peek() == '[')
            {
                return JSON_E_PROPERTY_NOT_REQUIRED;
            }
            else if(m_stack.Peek() == '{')
            {
                // we are in object
                // property can not be added multiple times
                //
                if(m_bPropertyAppended)
                {
                    return JSON_E_PROPERTY_ALREADY_ADDED;
                }
            }
            else
            {
                // we must be in array or object
                //
                return JSON_E_PROPERTY_ARRAY_OR_OBJECT_NOT_STARTED;
            }

            PrefixMemberSeperator();
            // You do not need seperator after property
            m_bFirstValue = true;
            m_strBuffer += PropertyStartToken;
            AppendEscapeString(pszStr);
            m_strBuffer += PropertyEndToken;
            m_strBuffer += ValueSeparatorToken;
            m_bPropertyAppended = true;
            return S_OK;
        }

        STDMETHODIMP StringValue(LPCWSTR pszStr)
        {
            if(!pszStr) return E_POINTER;

            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_strBuffer += StringStartToken;
            AppendEscapeString(pszStr);
            m_strBuffer += StringEndToken;
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP FragmentValue(LPCSTR pszFragment)
        {
            if(!pszFragment) return E_POINTER;

            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();

            m_strBuffer += pszFragment;
            if(FAILED(hr)) return hr;

            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP ObjectStart()
        {
            HRESULT hr = ValueAddCheck(true);
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_bFirstValue = true;
            hr = PushArrayOrObjectStart('{');
            if(FAILED(hr)) return hr;

            m_strBuffer += ObjectStartToken;
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP ObjectEnd()
        {
            // You can not start an object which is not started
            //
            if(!InArrayOrObject('{'))
            {
                return JSON_E_OBJECT_NOT_STARTED;
            }

            m_strBuffer += ObjectEndToken;
            m_bPropertyAppended = false;
            // You need to make firstvalue false, because you will need seperator in next call
            m_bFirstValue = false;
            return S_OK;
        }

        STDMETHODIMP ArrayStart()
        {
            HRESULT hr = ValueAddCheck(true);
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_bFirstValue = true;
            hr = PushArrayOrObjectStart('[');
            if(FAILED(hr)) return hr;
            m_strBuffer += ArrayStartToken;
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP ArrayEnd()
        {
            // You can not start an array which is not started
            //
            if(!InArrayOrObject('['))
            {
                return JSON_E_ARRAY_NOT_STARTED;
            }

            m_strBuffer += ArrayEndToken;
            m_bPropertyAppended = false;
            // You need to make firstvalue false, because you will need seperator in next call
            m_bFirstValue = false;
            return S_OK;
        }

        STDMETHODIMP IntValue(__int64 iVal)
        {
            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_strBuffer += ToString(iVal);
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP UIntValue(unsigned __int64 iVal)
        {
            HRESULT hr = ValueAddCheck();
            if (FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_strBuffer += ToString(iVal);
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP NumberValue(double dblVal)
        {
            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_strBuffer += ToString(dblVal);
            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP BoolValue(bool bVal)
        {
            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            if(bVal)
            {
                m_strBuffer += "true";
            }
            else
            {
                m_strBuffer += "false";
            }

            m_bPropertyAppended = false;
            return hr;
        }

        STDMETHODIMP NullValue()
        {
            HRESULT hr = ValueAddCheck();
            if(FAILED(hr)) return hr;

            PrefixMemberSeperator();
            m_strBuffer += "null";
            m_bPropertyAppended = false;
            return hr;
        }

        HRESULT Append(LPCWSTR pszValue)
        {
            ULONG nLen = (ULONG)wcslen(pszValue);
            for(ULONG nIndex=0; nIndex < nLen; nIndex++)
            {
                WCHAR c = pszValue[nIndex];
                AppendChar(c);
            }
            return S_OK;
        }

    private:

        void PrefixMemberSeperator()
        {
            if(!m_bFirstValue)
            {
                m_strBuffer += MemberSeparatorToken;
            }

            m_bFirstValue = false;
        }

        template<typename T>
        std::string ToString(T item)
        {
            std::string result;
            Common::StringWriterA(result).Write(item);
            return result;
        }

        // All control characters from 00 to 1F should be escaped.
        // 
        // In https://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf read the following para for more information
        // 
        // "A string is a sequence of Unicode code points wrapped with quotation marks (U+0022). All code points may be placed within the quotation marks
        // except for the code points that must be escaped: quotation mark (U+0022), reverse solidus (U+005C), and the control characters U+0000 to U+001F. 
        // There are two-character escape sequence representations of some characters."
        //
        char* GetEscapeSequence(WCHAR inputChar)
        {
            switch (inputChar)
            {
            case L'\\':
                return  "\\\\";
            case L'"':
                return  "\\\"";
            case L'/':
                return  "\\/";
            case L'\x00': 
                return "\\u0000";
            case L'\x01':
                return "\\u0001";
            case L'\x02':
                return "\\u0002";
            case L'\x03':
                return "\\u0003";
            case L'\x04':
                return "\\u0004";
            case L'\x05':
                return "\\u0005";
            case L'\x06':
                return "\\u0006";
            case L'\x07':
                return "\\u0007";
            case L'\b': // L'\x08'
                return  "\\b";
            case L'\f': // L'\x09'
                return  "\\f";
            case L'\n': // L'\x0A'
                return  "\\n";
            case L'\r': // L'\x0B'
                return  "\\r";
            case L'\v': // L'\x0C'
                return  "\\v";
            case L'\t': // L'\x0D'
                return  "\\t";
            case L'\x0E':
                return "\\u000E";
            case L'\x0F':
                return "\\u000F";
            case L'\x10':
                return "\\u0010";
            case L'\x11':
                return "\\u0011";
            case L'\x12':
                return "\\u0012";
            case L'\x13':
                return "\\u0013";
            case L'\x14':
                return "\\u0014";
            case L'\x15':
                return "\\u0015";
            case L'\x16':
                return "\\u0016";
            case L'\x17':
                return "\\u0017";
            case L'\x18':
                return "\\u0018";
            case L'\x19':
                return "\\u0019";
            case L'\x1A':
                return "\\u001A";
            case L'\x1B':
                return "\\u001B";
            case L'\x1C':
                return "\\u001C";
            case L'\x1D':
                return "\\u001D";
            case L'\x1E':
                return "\\u001E";
            case L'\x1F':
                return "\\u001F";
            default:
                return nullptr;
            }
        }

        void AppendChar(WCHAR c)
        {
            // encode 0 with two bytes
            if(c && c <= 0x7f)
            {
                m_strBuffer += (char)c;
            }
            else if(c <= 0x7ff)
            {
                char c1 = (char)(0xc0 | (c >> 6));
                char c2 = (char)(0x80 | (c & 0x3f));
                m_strBuffer += c1;
                m_strBuffer += c2;
            }
            else
            {
                char c1 = (char)(0xe0 | (c >> 12));
                char c2 = (char)(0x80 | ((c >> 6) & 0x3f));
                char c3 = (char)(0x80 | (c & 0x3f));
                m_strBuffer += c1;
                m_strBuffer += c2;
                m_strBuffer += c3;
            }
        }

        void AppendEscapeString(LPCWSTR pszStr)
        {
            for(int i = 0; pszStr[i] != '\0'; i++)
            {
                LPCSTR pszEscapedSequence = GetEscapeSequence(pszStr[i]);

                if(pszEscapedSequence  == NULL)
                {
                    AppendChar(pszStr[i]);
                }
                else
                {
                    m_strBuffer += pszEscapedSequence;
                }
            }
        }

        HRESULT PushArrayOrObjectStart(WCHAR charToPush)
        {
            if(m_stack.Push(charToPush) != S_OK)
            {
                return JSON_E_MAX_NESTING_EXCEEDED;
            }

            return S_OK;
        }

        HRESULT ValueAddCheck(bool bIsArrayOrObjectStart = false)
        {
            if(m_stack.IsEmpty())
            {
                if(m_bFirstValue)
                    return S_OK;
                else
                    return JSON_E_PROPERTY_ARRAY_OR_OBJECT_NOT_STARTED;
            }

            // inside a object check that propery must be added before addition of value
            //
            if(m_stack.Peek() == '{')
            {
                if(!m_bPropertyAppended)
                {
                    return JSON_E_MISSING_PROPERTY;
                }
            }
            else if(m_stack.Peek() == '[')
            {
                // ok we are in array
            }
            else
            {
                // starting a fresh array or object is correct
                //
                if(bIsArrayOrObjectStart)
                {
                    return S_OK;
                }
                else
                {
                    // we must be in array or object
                    //
                    return JSON_E_PROPERTY_ARRAY_OR_OBJECT_NOT_STARTED;
                }
            }

            return S_OK;
        }

        bool InArrayOrObject(WCHAR charToCheck)
        {
            WCHAR charInStack = m_stack.Peek();
            // check if passed input character is present on top of the stack, if it is present on top then pop it and return true,
            // otherwise do not pop from stack because we do not change stack in an error.
            //
            if(charInStack == charToCheck)
            {
                // pop
                //
                WCHAR tempChar;
                m_stack.Pop(&tempChar);
                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        // This method can be used by client to optimize the buffer allocation.
        // It's not specified by the IJsonWriter because it's specific to string builder.
        //
        HRESULT Preallocate(int iBufferPreAllocationLength)
        {
            // the user can reserve to larger sized buffer if he knows that string will be large, and he knows the approximate size.
            // it will help reallocation cost, if this value is not provided then we go with default buffer size.
            //
            if(iBufferPreAllocationLength > 0)
            {
                m_strBuffer.reserve(iBufferPreAllocationLength);
            }

            return S_OK;
        }

        // This method is for convenience to check if current json is well formed and correct according to the grammer.
        //
        bool IsComplete()
        {
            // json string generation is complete if the stack does not have any token left to process
            //
            return m_stack.IsEmpty();
        }

    private:
        std::string m_strBuffer;
        bool        m_bFirstValue;
        // It used to keep state that property is appened in previous call
        // we do not allow append of property just after a property is appened.
        bool        m_bPropertyAppended;
        JSONStack   m_stack;
    };
    //-------------------------------------------------------------------------------------------------------------------------------------------------
}
