// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    template <typename TBuf>
    bool TBufEqual(TBuf const & b1, TBuf const & b2)
    {
        return
            (b1.size() == b2.size()) &&
            !memcmp(b1.data(), b2.data(), b1.size());
    }

    template <typename TBuf>
    bool TBufLessThan(TBuf const & b1, TBuf const & b2)
    {
        if (b1.size() < b2.size()) return true;
        if (b1.size() > b2.size()) return false;

        // b1.size() == b2.size()

        return memcmp(b1.data(), b2.data(), b1.size()) < 0;
    }

    template <typename TBuf>
    TBuf StringToTBuf(std::wstring const& str)
    {
        TBuf buf(str.size() * sizeof(wchar_t));
        KMemCpySafe(buf.data(), buf.size(), str.data(), str.size() * sizeof(wchar_t));
        return buf;
    }

    bool operator == (ByteBuffer const & b1, ByteBuffer const & b2)
    {
        return TBufEqual<ByteBuffer>(b1, b2);
    }

    bool operator != (ByteBuffer const & b1, ByteBuffer const & b2)
    {
        return !(b1 == b2);
    }

    bool operator < (ByteBuffer const & b1, ByteBuffer const & b2)
    {
        return TBufLessThan<ByteBuffer>(b1, b2);
    }

    ByteBuffer StringToByteBuffer(std::wstring const& str)
    {
        return StringToTBuf<ByteBuffer>(str);
    }

    static const FormatOptions hexByteFormat(2, true, "x");

    wstring ByteBufferToString(ByteBuffer const & buf)
    {
        wstring str;
        StringWriter w(str);
        for (auto b : buf)
        {
            w.WriteNumber(b, hexByteFormat, false);
        }

        return str;
    }

    bool operator == (ByteBuffer2 const & b1, ByteBuffer2 const & b2)
    {
        return TBufEqual<ByteBuffer2>(b1, b2);
    }

    bool operator < (ByteBuffer2 const & b1, ByteBuffer2 const & b2)
    {
        return TBufLessThan<ByteBuffer2>(b1, b2);
    }

    ByteBuffer2 StringToByteBuffer2(std::wstring const& str)
    {
        return StringToTBuf<ByteBuffer2>(str);
    }

    void ByteBuffer2::WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const
    {
        w.Write("ptr=0x{0:x}, size={1}", TextTracePtr(data_), size_);

        if (option.formatString != "l") return;

        w.Write(", bytes=");
        for (auto ptr = data_; ptr < end(); ++ptr)
        {
            w.WriteNumber(*ptr, hexByteFormat, false);
        }
    }

    void const_buffer::WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const
    {
        w.Write("ptr=0x{0:x}, size={1}", TextTracePtr(buf), len);

        if (option.formatString != "l") return;

        w.Write(", bytes=");
        for (auto ptr = (unsigned char*)buf; ptr < ((unsigned char*)buf + len); ++ptr)
        {
            w.WriteNumber(*ptr, hexByteFormat, false);
        }
    }

#ifdef PLATFORM_UNIX

    template <typename TBuf>
    TBuf BioMemToTBuf(BIO* bio)
    {
        char* data = NULL;
        auto len = BIO_ctrl_pending(bio);
        Invariant(len >= 0);

        TBuf buf(len);
        auto read = BIO_read(bio, buf.data(), buf.size());
        Invariant(read == len);
        return buf; 
    }

    template <typename TBuf>
    BioUPtr TBufToBioMem(TBuf const & buffer)
    {
        BioUPtr bio(BIO_new(BIO_s_mem()));
        if (!bio)
        {
            auto error = LinuxCryptUtil().GetOpensslErr();
            Assert::CodingError(
                "ByteBufferToBioMem: BIO_new failed: {0}:{1}",
                error,
                error.Message);
        }

        BIO_write(bio.get(), buffer.data(), buffer.size());
        BIO_flush(bio.get());
        return bio; 
    }

    BioUPtr CreateBioMemFromBufferAndSize(BYTE const* buffer, size_t size)
    {
        BioUPtr bio(BIO_new(BIO_s_mem()));
        if (!bio)
        {
            auto error = LinuxCryptUtil().GetOpensslErr();
            Assert::CodingError(
                "ByteBufferToBioMem: BIO_new failed: {0}:{1}",
                error,
                error.Message);
        }

        BIO_write(bio.get(), buffer, size);
        BIO_flush(bio.get());
        return bio; 
    }

    ByteBuffer BioMemToByteBuffer(BIO* bio)
    {
        return BioMemToTBuf<ByteBuffer>(bio);
    }

    BioUPtr ByteBufferToBioMem(ByteBuffer const & buffer)
    {
        return TBufToBioMem(buffer);
    }

    ByteBuffer2 BioMemToByteBuffer2(BIO* bio)
    {
        return BioMemToTBuf<ByteBuffer2>(bio);
    }

    BioUPtr ByteBuffer2ToBioMem(ByteBuffer2 const & buffer)
    {
        return TBufToBioMem(buffer);
    }

    namespace
    {
        atomic_uint64 cryptObjCount(0); // tracks both x509 and private key objects

        struct CryptObjTracker 
        {
            static void* Track(void* obj);
            static void* Untrack(void* obj);
            static uint64 ObjCount();
        };

        uint64 CryptObjTracker::ObjCount()
        {
            return cryptObjCount.load();
        }

        void* CryptObjTracker::Track(void* obj)
        {
            if (!obj) return obj;

            auto count = ++cryptObjCount;
            Trace.WriteNoise(__FUNCTION__, "track obj {0}, object count = {1}", TextTracePtr(obj), count);
    //        StackTrace st;
    //        st.CaptureCurrentPosition();
    //        Trace.WriteInfo(__FUNCTION__, "track obj {0}, object count = {1}, stack:\r\n{2}", TextTracePtr(obj), count, st.ToString());
            return obj;
        }

        void* CryptObjTracker::Untrack(void* obj)
        {
            if (!obj) return obj;

            auto count = --cryptObjCount;
            Trace.WriteNoise(__FUNCTION__, "untrack obj {0}, object count = {1}", TextTracePtr(obj), count);
    //        StackTrace st;
    //        st.CaptureCurrentPosition();
    //        Trace.WriteInfo(__FUNCTION__, "untrack obj {0}, object count = {1}, stack:\r\n{2}", TextTracePtr(obj), count, st.ToString());
            return obj;
        }

        const StringLiteral CryptObjContextTrace("CryptObjContext");
    }

    template <typename T>
    CryptObjContext<T>::CryptObjContext(T* obj, string const & filePath) : obj_(obj), filePath_(move(filePath))
    {
        ASSERT(!deleter_);
        Trace.WriteNoise(CryptObjContextTrace, "{0}: ctor: obj_ = {1}, filePath_ = {2}", TextTraceThis, TextTracePtr(obj_), filePath_);
        CryptObjTracker::Track(obj_);
    }

    template <typename T>
    CryptObjContext<T>::CryptObjContext(CryptObjContext && other)
        : obj_(other.obj_), deleter_(other.deleter_), filePath_(move(other.filePath_))
    {
        Trace.WriteNoise(CryptObjContextTrace, "{0}: ctor: obj_ = {1}, filePath_ = {2}", TextTraceThis, TextTracePtr(obj_), filePath_);
        other.obj_ = nullptr;
    }

    template <typename T>
    CryptObjContext<T>::~CryptObjContext()
    {
        reset();
        Trace.WriteNoise(CryptObjContextTrace, "{0}: dtor", TextTraceThis);
    }

    template <typename T>
    void CryptObjContext<T>::reset()
    {
        Trace.WriteNoise(CryptObjContextTrace, "{0}: reset: {1}", TextTraceThis, TextTracePtr(obj_));
        if (!obj_) return;

        CryptObjTracker::Untrack(obj_);
        deleter_(obj_);
        obj_ = nullptr;
    }

    template <typename T>
    uint64 CryptObjContext<T>::ObjCount()
    {
        return CryptObjTracker::ObjCount();
    }

    template <typename T>
    CryptObjContext<T> & CryptObjContext<T>::operator=(CryptObjContext && other)
    {
        if (this == &other) return *this;

        reset();

        obj_ = other.obj_;
        other.obj_ = nullptr;

        deleter_ = other.deleter_;
        other.deleter_ = nullptr;

        filePath_ = move(other.filePath_);

        return *this;
    }

    template <typename T>
    T* CryptObjContext<T>::release()
    {
        Trace.WriteNoise(CryptObjContextTrace, "{0}: CryptObjContext release: {1}", TextTraceThis, TextTracePtr(obj_));
        filePath_.clear();

        auto obj = (T*)CryptObjTracker::Untrack(obj_);
        obj_ = nullptr;
        return obj;
    }

    X509Context::X509Context(X509* obj, string const & filePath) : CryptObjContext(obj, filePath)
    {
        deleter_ = X509_free;
    }

    PrivKeyContext::PrivKeyContext(EVP_PKEY* obj, string const & filePath) : CryptObjContext(obj, filePath)
    {
        deleter_ = EVP_PKEY_free;
    }

    bool operator == (timespec const & t1, timespec const & t2)
    {
        return (t1.tv_sec == t2.tv_sec) && (t1.tv_nsec == t2.tv_nsec);
    }

    bool operator != (timespec const & t1, timespec const & t2)
    {
        return !(t1 == t2);
    }

    bool operator < (timespec const & t1, timespec const & t2)
    {
        if (t1.tv_sec < t2.tv_sec) return true;

        if (t1.tv_sec > t2.tv_sec) return false;

        //t1.sec == t2.sec
        return t1.tv_nsec < t2.tv_nsec;
    }

    bool operator > (timespec const & t1, timespec const & t2)
    {
        if (t1.tv_sec > t2.tv_sec) return true;

        if (t1.tv_sec < t2.tv_sec) return false;

        //t1.sec == t2.sec
        return t1.tv_nsec > t2.tv_nsec;
    }

    bool operator <= (timespec const & t1, timespec const & t2)
    {
        return !(t1 > t2);
    }

    bool operator >= (timespec const & t1, timespec const & t2)
    {
        return !(t1 < t2);
    }

    bool timespec_is_zero(timespec const & ts)
    {
        return (ts.tv_sec == 0) && (ts.tv_nsec == 0);
    }

#endif

}

#ifdef PLATFORM_UNIX

template class Common::CryptObjContext<X509>;
template class Common::CryptObjContext<EVP_PKEY>;

#endif

