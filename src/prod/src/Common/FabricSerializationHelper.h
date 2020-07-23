// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <IFabricSerializableStream.h>
#include <type_traits>

struct FabricSerializationHelper;

template <class T>
struct FabricSerializationTraits
{
    static const bool IsBlittable = false;
};

#define DEFINE_AS_BLITTABLE(TYPE)           \
template <>                                 \
struct FabricSerializationTraits<TYPE>      \
{                                           \
    static const bool IsBlittable = true;   \
};                                          \

#define CheckReadFieldSuccess(_status) \
    if (NT_ERROR(_status) || K_STATUS_SCOPE_END == _status) \
    { \
        return _status; \
    } \

template <class T> struct FabricSerializableTraits
{
    static const bool IsSerializable = false;
};


template <bool B, class T>
struct EnumSerializer
{
    // Implementation is not needed since a bool is specialized for true and false
};

// We will serialize enums as LONG64
template <class T>
struct EnumSerializer<true, T>
{
    static NTSTATUS Serialize(FabricSerializationHelper * helper, T & t)
    {
        return helper->Write(static_cast<LONG64>(t));
    }

    static NTSTATUS Deserialize(FabricSerializationHelper * helper, T & t)
    {
        LONG64 temp;

        NTSTATUS _status = helper->Read(temp);
        CheckReadFieldSuccess(_status);
        t = static_cast<T>(temp);

        return _status;
    }
};

// This specialization is for non-enums and the type must have a definition for serializing
template <class T>
struct EnumSerializer<false, T>
{
    static NTSTATUS Serialize(FabricSerializationHelper * helper, T & t)
    {
        static_assert(FabricSerializableTraits<T>::IsSerializable, "Type T must declare a primitive to be serialized as");

        return FabricSerializableTraits<T>::FabricSerialize(helper, t);
    }

    static NTSTATUS Deserialize(FabricSerializationHelper * helper, T & t)
    {
        static_assert(FabricSerializableTraits<T>::IsSerializable, "Type T must declare a primitive to be serialized as");

        return FabricSerializableTraits<T>::FabricDeserialize(helper, t);

    }
};


#define FABRIC_SERIALIZE_AS_(USER_TYPE, PRIMITIVE_TYPE)                                         \
template <> struct FabricSerializableTraits<USER_TYPE>                                          \
{                                                                                               \
    static const bool IsSerializable = true;                                                    \
                                                                                                \
    static NTSTATUS FabricSerialize(FabricSerializationHelper * helper, USER_TYPE & t)          \
    {                                                                                           \
        return helper->Write(static_cast<PRIMITIVE_TYPE>(t));                                   \
    }                                                                                           \
                                                                                                \
    static NTSTATUS FabricDeserialize(FabricSerializationHelper * helper, USER_TYPE & t)        \
    {                                                                                           \
        PRIMITIVE_TYPE temp;                                                                    \
        NTSTATUS _status = helper->Read(temp);                                                   \
        CheckReadFieldSuccess(_status);                                                          \
        t = static_cast<USER_TYPE>(temp);                                                       \
        return _status;                                                                          \
    }                                                                                           \
};                                                                                              \

#define FABRIC_SERIALIZE_AS_CHAR(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, CHAR);
#define FABRIC_SERIALIZE_AS_UCHAR(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, UCHAR);

#define FABRIC_SERIALIZE_AS_SHORT(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, SHORT);
#define FABRIC_SERIALIZE_AS_USHORT(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, USHORT);

#define FABRIC_SERIALIZE_AS_LONG(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, LONG);
#define FABRIC_SERIALIZE_AS_ULONG(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, ULONG);

#define FABRIC_SERIALIZE_AS_LONG64(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, LONG64);
#define FABRIC_SERIALIZE_AS_ULONG64(USER_TYPE) FABRIC_SERIALIZE_AS_(USER_TYPE, ULONG64);

#define FABRIC_SERIALIZE_AS_BYTEARRAY(USER_TYPE)                                                \
DEFINE_AS_BLITTABLE(USER_TYPE)                                                                  \
template <> struct FabricSerializableTraits<USER_TYPE>                                          \
{                                                                                               \
    static const bool IsSerializable = true;                                                    \
                                                                                                \
    static NTSTATUS FabricSerialize(FabricSerializationHelper * helper, USER_TYPE & t)          \
    {                                                                                           \
        return helper->WriteByteArray(&t, sizeof(USER_TYPE));                                   \
    }                                                                                           \
                                                                                                \
    static NTSTATUS FabricDeserialize(FabricSerializationHelper * helper, USER_TYPE & t)        \
    {                                                                                           \
        return helper->ReadByteArray(&t, sizeof(USER_TYPE));                                    \
    }                                                                                           \
};                                                                                              \

template <class T>
struct ArrayUtility
{
    static NTSTATUS WriteArray(__in Serialization::IFabricSerializableStream * stream, __in T * data, __in ULONG count)
    {
        static_assert(FabricSerializationTraits<T>::IsBlittable, "This type T is not supported in a KArray/std::vector, consider using a DEFINE_USER_ARRAY_UTILITY or DEFINE_AS_BLITTABLE macro.");

        return stream->WriteUCharArray(reinterpret_cast<UCHAR*>(data), static_cast<ULONG>(count * sizeof(T)));
    }

    static NTSTATUS ReadArray(__in Serialization::IFabricSerializableStream * stream, __out T * data, __inout ULONG & count)
    {
        static_assert(FabricSerializationTraits<T>::IsBlittable, "This type T is not supported in a KArray/std::vector, consider using a DEFINE_USER_ARRAY_UTILITY or DEFINE_AS_BLITTABLE macro.");

        NTSTATUS _status;
        ULONG byteCount = static_cast<ULONG>(count * sizeof(T));

        if (byteCount == 0)
        {
            _status = stream->ReadUCharArray(nullptr, byteCount);

            count = byteCount / static_cast<ULONG>(sizeof(T));

            return _status;
        }

        _status = stream->ReadUCharArray(reinterpret_cast<UCHAR*>(data), byteCount);

        count = byteCount / static_cast<ULONG>(sizeof(T));

        return _status;
    }
};

#define DEFINE_ARRAY_UTILITY(TYPE, FUNCTIONNAME)                                                                \
template <> struct ArrayUtility<TYPE>                                                                           \
{                                                                                                               \
    static NTSTATUS WriteArray(__in Serialization::IFabricSerializableStream * stream, __in TYPE * data, __in ULONG count)     \
    {                                                                                                           \
        return stream->Write##FUNCTIONNAME##Array(data, count);                                                 \
    }                                                                                                           \
                                                                                                                \
    static NTSTATUS ReadArray(__in Serialization::IFabricSerializableStream * stream, __out TYPE * data, __inout ULONG & count)\
    {                                                                                                           \
        return stream->Read##FUNCTIONNAME##Array(data, count);                                                  \
    }                                                                                                           \
};                                                                                                              \

#define DEFINE_USER_ARRAY_UTILITY(TYPE)                                                                         \
template <> struct ArrayUtility<TYPE>                                                                           \
{                                                                                                               \
    static NTSTATUS WriteArray(__in Serialization::IFabricSerializableStream * stream, __in TYPE * data, __in ULONG count)     \
    {                                                                                                           \
        Serialization::IFabricSerializable ** ptrArray = new Serialization::IFabricSerializable*[count];        \
                                                                                                                \
        if (ptrArray == nullptr)                                                                                \
        {                                                                                                       \
            return STATUS_INSUFFICIENT_RESOURCES;                                                               \
        }                                                                                                       \
                                                                                                                \
        for (ULONG i = 0; i < count; ++i)                                                                       \
        {                                                                                                       \
            ptrArray[i] = &(data[i]);                                                                           \
        }                                                                                                       \
                                                                                                                \
        NTSTATUS _status = stream->WriteSerializableArray(ptrArray, count);                                      \
                                                                                                                \
        delete [] ptrArray;                                                                                     \
        ptrArray = nullptr;                                                                                     \
                                                                                                                \
        return _status;                                                                                          \
    }                                                                                                           \
                                                                                                                \
    static NTSTATUS ReadArray(__in Serialization::IFabricSerializableStream * stream, __out TYPE * data, __inout ULONG & count)\
    {                                                                                                           \
        if (count == 0)                                                                                         \
        {                                                                                                       \
            return stream->ReadSerializableArray(nullptr, count);                                               \
        }                                                                                                       \
                                                                                                                \
        Serialization::IFabricSerializable ** ptrArray = new Serialization::IFabricSerializable*[count];        \
                                                                                                                \
        if (ptrArray == nullptr)                                                                                \
        {                                                                                                       \
            return STATUS_INSUFFICIENT_RESOURCES;                                                               \
        }                                                                                                       \
                                                                                                                \
        for (ULONG i = 0; i < count; ++i)                                                                       \
        {                                                                                                       \
            ptrArray[i] = &(data[i]);                                                                           \
        }                                                                                                       \
                                                                                                                \
        NTSTATUS _status = stream->ReadSerializableArray(ptrArray, count);                                       \
                                                                                                                \
        delete [] ptrArray;                                                                                     \
        ptrArray = nullptr;                                                                                     \
                                                                                                                \
        return _status;                                                                                          \
    }                                                                                                           \
};                                                                                                              \

DEFINE_ARRAY_UTILITY(bool, Bool);
DEFINE_ARRAY_UTILITY(CHAR, Char);
DEFINE_ARRAY_UTILITY(UCHAR, UChar);
DEFINE_ARRAY_UTILITY(SHORT, Short);
DEFINE_ARRAY_UTILITY(USHORT, UShort);
DEFINE_ARRAY_UTILITY(LONG, Int32);
DEFINE_ARRAY_UTILITY(ULONG, UInt32);
DEFINE_ARRAY_UTILITY(LONG64, Int64);
DEFINE_ARRAY_UTILITY(ULONG64, UInt64);
DEFINE_ARRAY_UTILITY(GUID, Guid);
DEFINE_ARRAY_UTILITY(DOUBLE, Double);

template <class T>
Serialization::IFabricSerializable * DefaultActivator(Serialization::FabricTypeInformation)
{
    return new T();
}

// Below code was generated by a tool

#define CheckStatus(_status)         \
    if (NT_ERROR(_status))           \
    {                               \
        return _status;              \
    }                               \



#define FABRIC_FIELDS_01(ARG0) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_02(ARG0, ARG1) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_03(ARG0, ARG1, ARG2) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_04(ARG0, ARG1, ARG2, ARG3) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_05(ARG0, ARG1, ARG2, ARG3, ARG4) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_06(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_07(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_08(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_09(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_10(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_11(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_12(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_13(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_14(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_15(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_16(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_17(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_18(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_19(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_20(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_21(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_22(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_23(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_24(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_25(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_26(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \


#define FABRIC_FIELDS_27(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \

    #define FABRIC_FIELDS_28(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG27);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG27);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }     

    #define FABRIC_FIELDS_29(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG27);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG28);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG27);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG28);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \

    #define FABRIC_FIELDS_30(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG27);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG28);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG29);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG27);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG28);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG29);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }     

    #define FABRIC_FIELDS_31(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29, ARG30) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG27);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG28);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG29);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG30);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG27);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG28);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG29);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG30);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \

    #define FABRIC_FIELDS_30(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24, ARG25, ARG26, ARG27, ARG28, ARG29) \
    virtual NTSTATUS Write(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Write(stream);              \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.WriteStartType();                \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG0);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG1);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG2);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG3);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG4);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG5);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG6);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG7);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG8);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG9);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG10);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG11);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG12);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG13);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG14);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG15);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG16);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG17);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG18);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG19);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG20);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG21);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG22);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG23);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG24);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG25);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG26);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG27);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG28);                    \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Write(ARG29);                    \
        CheckStatus(_status);                                   \
                                                                \
        return streamHelper.WriteEndType();                     \
    }                                                           \
                                                                \
    virtual NTSTATUS Read(Serialization::IFabricSerializableStream * stream) \
    {                                                           \
        NTSTATUS _status = __super::Read(stream);               \
        CheckStatus(_status);                                   \
                                                                \
        FabricSerializationHelper streamHelper(stream);         \
                                                                \
        _status = streamHelper.ReadStartType();                 \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG0);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG1);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG2);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG3);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG4);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG5);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG6);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG7);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG8);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG9);                      \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG10);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG11);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG12);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG13);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG14);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG15);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG16);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG17);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG18);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG19);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG20);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG21);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG22);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG23);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG24);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG25);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG26);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG27);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG28);                     \
        CheckStatus(_status);                                   \
                                                                \
        _status = streamHelper.Read(ARG29);                     \
        CheckStatus(_status);                                   \
                                                                \
        return stream->ReadEndType();                           \
    }                                                           \

#define FABRIC_PRIMITIVE_FIELDS_01(ARG0)                    \
NTSTATUS FabricSerialize(FabricSerializationHelper * helper)\
{                                                           \
    NTSTATUS _status = helper->Write(ARG0);                 \
    return _status;                                         \
}                                                           \
                                                            \
NTSTATUS FabricDeserialize(FabricSerializationHelper * helper) \
{                                                           \
    NTSTATUS _status = helper->Read(ARG0);                  \
    return _status;                                         \
}                                                           \

#define FABRIC_PRIMITIVE_FIELDS_02(ARG0, ARG1)              \
NTSTATUS FabricSerialize(FabricSerializationHelper * helper)\
{                                                           \
    NTSTATUS _status = helper->Write(ARG0);                 \
    CheckStatus(_status);                                   \
    _status = helper->Write(ARG1);                          \
    return _status;                                         \
}                                                           \
                                                            \
NTSTATUS FabricDeserialize(FabricSerializationHelper * helper) \
{                                                           \
    NTSTATUS _status = helper->Read(ARG0);                  \
    CheckStatus(_status);                                   \
    _status = helper->Read(ARG1);                           \
    return _status;                                         \
}                                                           \

#define FABRIC_PRIMITIVE_FIELDS_03(ARG0, ARG1, ARG2)        \
NTSTATUS FabricSerialize(FabricSerializationHelper * helper)\
{                                                           \
    NTSTATUS _status = helper->Write(ARG0);                 \
    CheckStatus(_status);                                   \
    _status = helper->Write(ARG1);                          \
    CheckStatus(_status);                                   \
    _status = helper->Write(ARG2);                          \
    return _status;                                         \
}                                                           \
                                                            \
NTSTATUS FabricDeserialize(FabricSerializationHelper * helper) \
{                                                           \
    NTSTATUS _status = helper->Read(ARG0);                  \
    CheckStatus(_status);                                   \
    _status = helper->Read(ARG1);                           \
    CheckStatus(_status);                                   \
    _status = helper->Read(ARG2);                           \
    return _status;                                         \
}                                                           \

#define DEFINE_WRITE_READ(TYPE, FUNCTIONTYPE, FUNCTIONNAME)                                 \
    NTSTATUS Write(TYPE field) { return this->stream_->Write##FUNCTIONNAME  (field); }      \
                                                                                            \
    NTSTATUS Read(TYPE & field)                                                             \
    {                                                                                       \
        FUNCTIONTYPE temp;                                                                  \
                                                                                            \
        NTSTATUS _status =  this->stream_->Read##FUNCTIONNAME  (temp);                      \
        CheckReadFieldSuccess(_status);                                                     \
        field = temp;                                                                       \
                                                                                            \
        return _status;                                                                     \
    }                                                                                       \

struct FabricSerializationHelper
{
    FabricSerializationHelper(FabricSerializationHelper const &);
    FabricSerializationHelper & operator=(FabricSerializationHelper const &);

public:
    FabricSerializationHelper(Serialization::IFabricSerializableStream * stream)
        : stream_(stream)
    {
    }

    NTSTATUS Write(Serialization::IFabricSerializable * object)
    {
        return this->stream_->WritePointer(object);
    }

    NTSTATUS Read(__out Serialization::IFabricSerializable ** object, __in  Serialization::FabricActivatorFunction activator)
    {
        return this->stream_->ReadPointer(object, activator);
    }

    template <class T>
    NTSTATUS Write(std::deque<T> & field)
    {
        std::vector<T> items;

        for(T item : field)
        {
            items.push_back(item);
        }

        return this->Write(items);
    }

    template <class T>
    NTSTATUS Read(std::deque<T> & field)
    {
        std::vector<T> items;

        NTSTATUS _status = this->Read(items);

        CheckReadFieldSuccess(_status);

        field.clear();

        for(T item : items)
        {
            field.push_back(item);
        }

        return _status;
    }    
    
    template <class TKey, class TValue>
    NTSTATUS Write(std::map<TKey, TValue> & field)
    {
        std::vector<ArrayPair<TKey, TValue>> items;

        for(auto &item : field)
        {
            ArrayPair<TKey, TValue> arrayPair;

            arrayPair.key = item.first;
            arrayPair.value = item.second;

            items.push_back(arrayPair);
        }

        return this->Write(items);
    }

    template <class TKey, class TValue>
    NTSTATUS Read(std::map<TKey, TValue> & field)
    {
        std::vector<ArrayPair<TKey, TValue>> items;

        NTSTATUS _status = this->Read(items);

        CheckReadFieldSuccess(_status);

        field.clear();

        for(auto & arrayPair : items)
        {
            std::pair<TKey, TValue> item;

            item.first = arrayPair.key;
            item.second = arrayPair.value;

            field.insert(item);
        }

        return _status;
    }

    template <class TKey, class TValue>
    NTSTATUS Write(KHashTable<TKey, TValue> & field)
    {
        field.Reset();

        std::vector<ArrayPair<TKey, TValue>> items;

        NTSTATUS _status = STATUS_SUCCESS;
        
        do
        {
            ArrayPair<TKey, TValue> arrayPair;

            _status = field.Next(arrayPair.key, arrayPair.value);

            if (NT_SUCCESS(_status))
            {
                items.push_back(arrayPair);
            }

        } while (NT_SUCCESS(_status));

        return this->Write(items);
    }

    template <class TKey, class TValue>
    NTSTATUS Read(KHashTable<TKey, TValue> & field)
    {
        std::vector<ArrayPair<TKey, TValue>> items;

        NTSTATUS _status = this->Read(items);

        CheckReadFieldSuccess(_status);

        field.Clear();

        for (ArrayPair<TKey, TValue> const & arrayPair : items)
        {
            _status = field.Put((TKey&)arrayPair.key, (TValue&)arrayPair.value, TRUE);

            CheckReadFieldSuccess(_status);
        }

        return _status;
    }

    template <class TElem>
    NTSTATUS Write(std::set<TElem> & field)
    {
        std::vector<TElem> items;

        for (TElem const & item : field)
        {
            items.push_back(item);
        }

        return this->Write(items);
    }

    template <class TElem>
    NTSTATUS Read(std::set<TElem> & field)
    {
        std::vector<TElem> items;

        NTSTATUS _status = this->Read(items);

        CheckReadFieldSuccess(_status);

        field.clear();

        for (TElem const & item : items)
        {
            field.insert(item);
        }

        return _status;
    }

	template <class TElem>
	NTSTATUS Write(std::unordered_set<TElem> & field)
	{
		std::vector<TElem> items;

		for (TElem const & item : field)
		{
			items.push_back(item);
		}

		return this->Write(items);
	}

	template <class TElem>
	NTSTATUS Read(std::unordered_set<TElem> & field)
	{
		std::vector<TElem> items;

		NTSTATUS _status = this->Read(items);

		CheckReadFieldSuccess(_status);

		field.clear();

		for (TElem const & item : items)
		{
			field.insert(item);
		}

		return _status;
	}

    template <class T>
    NTSTATUS Write(std::unique_ptr<T> & object)
    {
        return this->stream_->WritePointer(&(*object));
    }

    template <class T>
    NTSTATUS Read(std::unique_ptr<T> & object, __in  Serialization::FabricActivatorFunction activator = nullptr)
    {
        Serialization::IFabricSerializable * pointer = nullptr;

        if (activator == nullptr)
        {
            __if_exists(T::FabricSerializerActivator)
            {
                activator = T::FabricSerializerActivator;
            }
            __if_not_exists(T::FabricSerializerActivator)
            {
                activator = DefaultActivator<T>;
            }
        }

        NTSTATUS _status = this->stream_->ReadPointer(&pointer, activator);

        CheckReadFieldSuccess(_status);

        object = std::unique_ptr<T>(static_cast<T*>(pointer));

        return _status;
    }

    template <class T>
    NTSTATUS Write(std::shared_ptr<T> & object)
    {
        __if_exists(T::GetTypeInformation)
        {
#if defined(PLATFORM_UNIX)
            return ((object == nullptr) ? this->stream_->WritePointer(0) : this->stream_->WritePointer(&(*object)));
#else
            return this->stream_->WritePointer(&(*object));
#endif
        }
        __if_not_exists(T::GetTypeInformation)
        {
            std::shared_ptr<PointerWrapper<T>> wrapper;
            if (object) 
            { 
                wrapper = std::make_shared<PointerWrapper<T>>(*object); 
            }

            return this->stream_->WritePointer(&(*wrapper));
        }
    }

//shared_ptr<bool> is needed for nullable bool
#if defined(PLATFORM_UNIX)
    template <>
    NTSTATUS Write(std::shared_ptr<bool> & object)
    {
        std::shared_ptr<PointerWrapper<bool>> wrapper;
        if (object)
        {
            wrapper = std::make_shared<PointerWrapper<bool>>(*object);
        }

        return this->stream_->WritePointer(&(*wrapper));
    }
#endif

    template <class T>
    NTSTATUS Read(std::shared_ptr<T> & object, __in  Serialization::FabricActivatorFunction activator = nullptr)
    {
        Serialization::IFabricSerializable * pointer = nullptr;
        NTSTATUS _status = STATUS_SUCCESS;

        __if_exists(T::GetTypeInformation)
        {
            if (activator == nullptr)
            {
                __if_exists(T::FabricSerializerActivator)
                {
                    activator = T::FabricSerializerActivator;
                }
                __if_not_exists(T::FabricSerializerActivator)
                {
                    activator = DefaultActivator<T>;
                }
            }

            _status = this->stream_->ReadPointer(&pointer, activator);
            
            if (NT_SUCCESS(_status))
            {
                object = std::shared_ptr<T>(static_cast<T*>(pointer));
            }
        }
        __if_not_exists(T::GetTypeInformation)
        {
            if (activator == nullptr)
            {
                activator = DefaultActivator<PointerWrapper<T>>;
            }

            _status = this->stream_->ReadPointer(&pointer, activator);

            if (NT_SUCCESS(_status))
            {
                if (pointer != nullptr)
                {
                    object = std::make_shared<T>(static_cast<PointerWrapper<T>*>(pointer)->GetValue());
                }
                else
                {
                    object = std::shared_ptr<T>();
                }
            }
        }

        return _status;
    }
#if defined(PLATFORM_UNIX)
    template <>
    NTSTATUS Read<bool>(std::shared_ptr<bool> & object, __in  Serialization::FabricActivatorFunction activator)
    {
        Serialization::IFabricSerializable * pointer = nullptr;
        NTSTATUS _status;
        if (activator == nullptr)
        {
            activator = DefaultActivator < PointerWrapper<bool> >;
        }
        _status = this->stream_->ReadPointer(&pointer, activator);
        if (NT_SUCCESS(_status))
        {
            if (pointer != nullptr)
            {
                object = std::make_shared<bool>(static_cast<PointerWrapper<bool>*>(pointer)->GetValue());
            }
            else
            {
                object = std::shared_ptr<bool>();
            }
        }
        return _status;
    }

    NTSTATUS Read(std::shared_ptr<bool> & object)
    {
        return Read(object, nullptr);
    }
#endif

    template <class T>
    NTSTATUS Write(std::vector<std::shared_ptr<T>> & field)
    {
        NTSTATUS _status = this->Write(static_cast<ULONG>(field.size()));

        if (NT_ERROR(_status))
        {
            return _status;
        }

        for (ULONG i = 0; i < field.size(); ++i)
        {
            _status = this->Write(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }

    template <class T>
    NTSTATUS Write(std::vector<std::unique_ptr<T>> & field)
    {
        NTSTATUS _status = this->Write(static_cast<ULONG>(field.size()));

        if (NT_ERROR(_status))
        {
            return _status;
        }

        for (ULONG i = 0; i < field.size(); ++i)
        {
            _status = this->Write(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }

    template <class T>
    NTSTATUS Read(std::vector<std::shared_ptr<T>> & field)
    {
        ULONG size = 0;

        NTSTATUS _status = this->Read(size);

        CheckReadFieldSuccess(_status);

        field.resize(size);

        for (ULONG i = 0; i < size; ++i)
        {
            _status = this->Read(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }

    template <class T>
    NTSTATUS Read(std::vector<std::unique_ptr<T>> & field)
    {
        ULONG size = 0;

        NTSTATUS _status = this->Read(size);

        CheckReadFieldSuccess(_status);

        field.resize(size);

        for (ULONG i = 0; i < size; ++i)
        {
            _status = this->Read(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }
    template<class T>
    NTSTATUS Write(T & object)
    {
        __if_exists(T::GetTypeInformation)
        {
            return this->stream_->WriteSerializable(&object);
        }
        __if_not_exists(T::GetTypeInformation)
        {
            __if_exists(T::FabricSerialize)
            {
                return object.FabricSerialize(this);
            }
            __if_not_exists(T::FabricSerialize)
            {
                // This object does not define serialization.  This specialization will serialize enums correctly with compression,
                // anything else must have a special serialize as type or be blittable
                return EnumSerializer<std::is_enum<T>::value, T>::Serialize(this, object);
            }
        }
    }

    template<class T>
    NTSTATUS Read(T & object)
    {
        __if_exists(T::GetTypeInformation)
        {
            return this->stream_->ReadSerializable(&object);
        }
        __if_not_exists(T::GetTypeInformation)
        {
            __if_exists(T::FabricDeserialize)
            {
                return object.FabricDeserialize(this);
            }
            __if_not_exists(T::FabricDeserialize)
            {
                return EnumSerializer<std::is_enum<T>::value, T>::Deserialize(this, object);
            }
        }
    }

    NTSTATUS Write(Serialization::IFabricSerializable & object)
    {
        return this->stream_->WriteSerializable(&object);
    }

    NTSTATUS Read(Serialization::IFabricSerializable & object)
    {
        return this->stream_->ReadSerializable(&object);
    }

    NTSTATUS Write(std::vector<std::wstring> & field)
    {
        NTSTATUS _status = this->Write(static_cast<ULONG>(field.size()));

        if (NT_ERROR(_status))
        {
            return _status;
        }

        for (ULONG i = 0; i < field.size(); ++i)
        {
            _status = this->Write(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }

    NTSTATUS Read(std::vector<std::wstring> & field)
    {
        ULONG size = 0;

        NTSTATUS _status = this->Read(size);

        CheckReadFieldSuccess(_status);

        field.resize(size);

        for (ULONG i = 0; i < size; ++i)
        {
            _status = this->Read(field[i]);

            if (NT_ERROR(_status))
            {
                return _status;
            }
        }

        return _status;
    }


    NTSTATUS Write(std::wstring & field)
    {
        return this->stream_->WriteString(const_cast<wchar_t*>(field.c_str()), static_cast<ULONG>(field.size()));
    }

    NTSTATUS Read(std::wstring & field)
    {
        ULONG size = 0;

        NTSTATUS _status = this->stream_->ReadString(nullptr, size);

        if (K_STATUS_BUFFER_TOO_SMALL == _status)
        {
            field.resize(size);
        }
        else if (NT_SUCCESS(_status) && K_STATUS_SCOPE_END != _status)
        {
            field = L"";
            return STATUS_SUCCESS;
        }
        else
        {
            return _status;
        }

        return this->stream_->ReadString(const_cast<wchar_t*>(field.c_str()), size);
    }

    NTSTATUS WriteByteArray(void * field, ULONG size)
    {
        return this->stream_->WriteUCharArray(static_cast<UCHAR*>(field), size);
    }

    NTSTATUS ReadByteArray(void * field, ULONG size)
    {
        NTSTATUS _status = this->stream_->ReadUCharArray(static_cast<UCHAR*>(field), size);

        if (K_STATUS_BUFFER_TOO_SMALL == _status)
        {
            return K_STATUS_INVALID_STREAM_FORMAT;
        }

        return _status;
    }

    NTSTATUS Write(std::vector<byte> & field)
    {
        return this->stream_->WriteUCharArray(const_cast<UCHAR*>(field.data()), static_cast<ULONG>(field.size()));
    }

    NTSTATUS Read(std::vector<byte> & field)
    {
        ULONG size = 0;

        NTSTATUS _status = this->stream_->ReadUCharArray(nullptr, size);

        if (K_STATUS_BUFFER_TOO_SMALL == _status)
        {
            field.resize(size);
        }
        else if (NT_SUCCESS(_status) && K_STATUS_SCOPE_END != _status)
        {
            field.clear();
            return STATUS_SUCCESS;
        }
        else
        {
            return _status;
        }

        return this->stream_->ReadUCharArray(const_cast<UCHAR*>(field.data()), size);
    }

    template <class T>
    NTSTATUS Write(std::vector<T> & field)
    {
        ULONG size = static_cast<ULONG>(field.size());
        return ArrayUtility<T>::WriteArray(this->stream_, field.data(), size);
    }

    template <class T>
    NTSTATUS Read(std::vector<T> & field)
    {
        field.resize(field.capacity());
        ULONG size = static_cast<ULONG>(field.capacity());
        NTSTATUS _status = ArrayUtility<T>::ReadArray(this->stream_, field.data(), size);

        if (K_STATUS_BUFFER_TOO_SMALL == _status)
        {
            field.resize(size);
        }
        else if (NT_SUCCESS(_status) && K_STATUS_SCOPE_END != _status)
        {
            field.resize(size);

            return STATUS_SUCCESS;
        }
        else
        {
            return _status;
        }

        size = static_cast<ULONG>(field.size());

        _status = ArrayUtility<T>::ReadArray(this->stream_, field.data(), size);

        if (NT_ERROR(_status))
        {
            return _status;
        }

        return _status;
    }

    template <class T>
    NTSTATUS Write(KArray<T> & field)
    {
        ULONG size = field.Count();
        return ArrayUtility<T>::WriteArray(this->stream_, field.Data(), size);
    }

    template <class T>
    NTSTATUS Read(KArray<T> & field)
    {
        field.SetCount(field.Max());
        ULONG size = field.Count();
        NTSTATUS _status = ArrayUtility<T>::ReadArray(this->stream_, field.Data(), size);

        if (K_STATUS_BUFFER_TOO_SMALL == _status)
        {
            CheckStatus(field.Reserve(size));
            ASSERT_IFNOT(TRUE == field.SetCount(size), "Setting the size of the array to the reserved size failed");
        }
        else if (NT_SUCCESS(_status) && K_STATUS_SCOPE_END != _status)
        {
            ASSERT_IFNOT(TRUE == field.SetCount(size), "Reducing the size of the array failed.");

            return STATUS_SUCCESS;
        }
        else
        {
            return _status;
        }

        size = field.Count();

        _status = ArrayUtility<T>::ReadArray(this->stream_, field.Data(), size);

        if (NT_ERROR(_status))
        {
            return _status;
        }

        return _status;
    }

    NTSTATUS WriteStartType() { return this->stream_->WriteStartType(); };

    NTSTATUS WriteEndType(Serialization::FabricCompletionCallback callback = nullptr, VOID * state = nullptr) { return this->stream_->WriteEndType(callback, state); };

    NTSTATUS Write(bool field) { return this->stream_->WriteBool(field); };

    NTSTATUS Write(CHAR field) { return this->stream_->WriteChar(field); };

    NTSTATUS Write(UCHAR field) { return this->stream_->WriteUChar(field); };

    NTSTATUS Write(SHORT field) { return this->stream_->WriteShort(field); };

    NTSTATUS Write(USHORT field) { return this->stream_->WriteUShort(field); };

    NTSTATUS Write(LONG field) { return this->stream_->WriteInt32(field); };

    NTSTATUS Write(ULONG field) { return this->stream_->WriteUInt32(field); };

    NTSTATUS Write(LONG64 field) { return this->stream_->WriteInt64(field); };

    NTSTATUS Write(ULONG64 field) { return this->stream_->WriteUInt64(field); };

    NTSTATUS Write(GUID field) { return this->stream_->WriteGuid(field); };

    NTSTATUS Write(DOUBLE field) { return this->stream_->WriteDouble(field); };

    NTSTATUS ReadStartType() { return this->stream_->ReadStartType(); };

    NTSTATUS ReadEndType() { return this->stream_->ReadEndType(); };

    NTSTATUS Read(bool & field) { return this->stream_->ReadBool(field); };

    NTSTATUS Read(CHAR & field) { return this->stream_->ReadChar(field); };

    NTSTATUS Read(UCHAR & field) { return this->stream_->ReadUChar(field); };

    NTSTATUS Read(SHORT & field) { return this->stream_->ReadShort(field); };

    NTSTATUS Read(USHORT & field) { return this->stream_->ReadUShort(field); };

    NTSTATUS Read(LONG & field) { return this->stream_->ReadInt32(field); };

    NTSTATUS Read(ULONG & field) { return this->stream_->ReadUInt32(field); };

    NTSTATUS Read(LONG64 & field) { return this->stream_->ReadInt64(field); };

    NTSTATUS Read(ULONG64 & field) { return this->stream_->ReadUInt64(field); };

    NTSTATUS Read(GUID & field) { return this->stream_->ReadGuid(field); };

    NTSTATUS Read(DOUBLE & field) { return this->stream_->ReadDouble(field); };

    // Other typedefs for things defined in Types.h
#if !defined(PLATFORM_UNIX)
    DEFINE_WRITE_READ(int, LONG, Int32);
    DEFINE_WRITE_READ(uint, ULONG, UInt32);
#endif

private:

    // For supporting serialization of types that are not IFabricSerializable
    //
    template <typename T>
    class PointerWrapper : public Serialization::FabricSerializable
    {
    public:
        PointerWrapper() : field_() { }
        explicit PointerWrapper(T const & value) : field_(value) { }

        T GetValue() const { return field_; }

        FABRIC_FIELDS_01(field_);

    private:
        T field_;
    };

private:

    Serialization::IFabricSerializableStream * stream_;
};

// Moving ArrayPair down here for using the constructor
template <class TKey, class TValue>
struct ArrayPair : public Serialization::FabricSerializable
{
    TKey key;
    TValue value;

    FABRIC_FIELDS_02(key, value);
};

#if defined(PLATFORM_UNIX)
// ArrayPair::IsBlittable is false in clang compilation.
template <class TKey, class TValue>
struct FabricSerializationTraits<ArrayPair<TKey, TValue> >
{
    static const bool IsBlittable = true;
};
#endif

// Without this macro, the comma has issues in the template
#define DEFINE_ARRAY_PAIR(TKey, TValue)  ArrayPair<TKey, TValue>
#define DEFINE_USER_MAP_UTILITY(TKey, TValue)   DEFINE_USER_ARRAY_UTILITY(DEFINE_ARRAY_PAIR(TKey, TValue));

DEFINE_USER_MAP_UTILITY(std::wstring, std::wstring);

