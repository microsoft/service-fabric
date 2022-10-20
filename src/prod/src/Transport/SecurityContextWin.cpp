// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static const StringLiteral TraceType("SecurityContextWin");
static GENERIC_MAPPING genericMapping = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };

SecurityContextWin::SecurityContextWin(
    IConnectionSPtr const & connection,
    TransportSecuritySPtr const & transportSecurity,
    wstring const & targetName,
    ListenInstance localListenInstance)
    : SecurityContext(connection, transportSecurity, targetName, localListenInstance)
{
    RtlZeroMemory(&sizes_, sizeof(sizes_));

    if (inbound_)
    {
        securityRequirements_ |= ISC_REQ_MUTUAL_AUTH;
    }
}

SecurityContextWin::~SecurityContextWin()
{
    if (remoteToken_ != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(remoteToken_);
    }
}

void SecurityContextWin::OnInitialize()
{
    vector<SecurityCredentialsSPtr> dummy;
    transportSecurity_->GetCredentials(credentials_, dummy);
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::OnNegotiateSecurityContext(
    MessageUPtr const & inputMessage,
    PSecBufferDesc pOutput,
    MessageUPtr & outputMessage)
{
    vector<byte> input = GetMergedMessageBody(inputMessage);

    SecBufferDesc inputBufferDesc;
    SecBuffer inputBuffers[1];

    inputBufferDesc.ulVersion = SECBUFFER_VERSION;
    inputBufferDesc.cBuffers = 1;
    inputBufferDesc.pBuffers = inputBuffers;
    inputBuffers[0].BufferType = SECBUFFER_TOKEN;
    inputBuffers[0].cbBuffer = static_cast<ULONG>(input.size());
    inputBuffers[0].pvBuffer = input.data();

    TimeStamp timestamp;
#ifndef PLATFORM_UNIX
    if (inbound_)
    {
        auto api = [&] {
            negotiationState_ = ::AcceptSecurityContext(
                credentials_.front()->Credentials(),
                AscInputContext(),
                &inputBufferDesc,
                securityRequirements_,
                SECURITY_NETWORK_DREP,
                &hSecurityContext_,
                pOutput,
                &fContextAttrs_,
                &timestamp);
        };

        CheckCallDuration<TraceTaskCodes::Transport>(
            api,
            SecurityConfig::GetConfig().SlowApiThreshold,
            TraceType,
            id_,
            L"AcceptSecurityContext",
            [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });
    }
    else
    {
        auto api = [&]
        {
            negotiationState_ = ::InitializeSecurityContext(
                credentials_.front()->Credentials(),
                IscInputContext(),
                TargetName(),
                securityRequirements_,
                0,
                SECURITY_NETWORK_DREP,
                &inputBufferDesc,
                0,
                &hSecurityContext_,
                pOutput,
                &fContextAttrs_,
                &timestamp);
        };

        CheckCallDuration<TraceTaskCodes::Transport>(
            api,
            SecurityConfig::GetConfig().SlowApiThreshold,
            TraceType,
            id_,
            L"InitializeSecurityContext",
            [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });
    }
#endif

    if (NegotiationSucceeded())
    {
        auto error = SecurityCredentials::LocalTimeStampToDateTime(timestamp, contextExpiration_);
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType, id_,
                "failed to convert context expiration 0x{0:x}: {1}",
                timestamp.QuadPart,
                error);

            negotiationState_ = error.ToHResult();
            return negotiationState_;
        }
    }

    outputMessage = CreateSecurityNegoMessage(pOutput);
    return negotiationState_;
}

SECURITY_STATUS SecurityContextWin::EncodeMessage(TcpFrameHeader const & frameHeader, Message & message, ByteBuffer2 & encrypted)
{
    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        return EncryptMessage(frameHeader, message, encrypted);
    }

    Invariant(transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign);
    return SignMessage(frameHeader, message, encrypted);
}

SECURITY_STATUS SecurityContextWin::DecodeMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & output)
{
    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        return DecryptMessage(receiveQueue, output);
    }

    Invariant(transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign);
    return VerifySignedMessage(receiveQueue, output);
}

namespace
{
    struct SecBufferSizes
    {
        SecBufferSizes() = default;

        SecBufferSizes(unsigned int data, unsigned short padding, unsigned short token)
            : dataSize(data)
            , paddingSize(padding)
            , tokenSize(token)
        {
        }

        unsigned int dataSize = 0;
        unsigned short paddingSize = 0;
        unsigned short tokenSize = 0;
    };
}

SECURITY_STATUS SecurityContextWin::EncryptMessage(TcpFrameHeader const & frameHeader, Message & message, ByteBuffer2 & encrypted)
{
    auto encryptCount = (frameHeader.FrameLength() + maxEncryptSize_ - 1) / maxEncryptSize_;
    auto chunkOverhead = uint(sizeof(SecBufferSizes)) + sizes_.cbSecurityTrailer + sizes_.cbBlockSize;
    auto outputLimit = frameHeader.FrameLength() + chunkOverhead * encryptCount;
    if (outputLimit > numeric_limits<uint>::max())
    {
        WriteError(TraceType, id_, "EncryptMessage: too much to encrypt: {0}", outputLimit);
        return E_FAIL;
    }

    encrypted.resize(outputLimit);

    vector<MutableBuffer> chunks;
    chunks.reserve(64);
    chunks.emplace_back((void*)&frameHeader, sizeof(frameHeader));

    for (BiqueChunkIterator chunk = message.BeginHeaderChunks(); chunk != message.EndHeaderChunks(); ++chunk)
    {
        chunks.emplace_back((void*)chunk->cbegin(), chunk->size());
    }

    for (BufferIterator chunk = message.BeginBodyChunks(); chunk != message.EndBodyChunks(); ++chunk)
    {
        chunks.emplace_back((void*)chunk->cbegin(), chunk->size());
    }

    ByteBuffer2 trailer(sizes_.cbSecurityTrailer);
    auto padding = alloca(sizes_.cbBlockSize);

    auto iter = chunks.cbegin();
    uint chunkOffset = 0;
    while (iter != chunks.cend())
    {
        auto sizesPtr = encrypted.AppendCursor();
        auto dataPtr = encrypted.AdvanceAppendCursor(sizeof(SecBufferSizes));

        uint dataSize = 0;
        do
        {
            auto chunkStart = iter->buf + chunkOffset;
            auto chunkSize = iter->len - chunkOffset;
            if ((dataSize + chunkSize) > (maxEncryptSize_ - chunkOverhead))
            {
                auto toCopy = maxEncryptSize_ - chunkOverhead - dataSize;
                encrypted.append(chunkStart, toCopy);
                chunkOffset += toCopy;
                dataSize = maxEncryptSize_ - chunkOverhead;
                break;
            }

            encrypted.append(chunkStart, chunkSize); // todo, leikong, consider avoiding copy by using a list of output buffers
            dataSize += chunkSize;
            ++iter;
            chunkOffset = 0;
        } while (iter != chunks.cend());

        SecBuffer buffers[3];
        buffers[0].BufferType = SECBUFFER_TOKEN;
        buffers[0].pvBuffer = trailer.data();
        buffers[0].cbBuffer = (uint)trailer.size();

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = dataPtr;
        buffers[1].cbBuffer = dataSize;

        buffers[2].BufferType = SECBUFFER_PADDING;
        buffers[2].pvBuffer = padding;
        buffers[2].cbBuffer = sizes_.cbBlockSize;

        SecBufferDesc secBufferDesc;
        secBufferDesc.ulVersion = SECBUFFER_VERSION;
        secBufferDesc.pBuffers = buffers;
        secBufferDesc.cBuffers = 3;

        SECURITY_STATUS status = ::EncryptMessage(&hSecurityContext_, 0, &secBufferDesc, 0);
        if (FAILED(status))
        {
            WriteError(TraceType, id_, "EncryptMessage failed: 0x{0:x}", (uint)status);
            return status;
        }

        auto paddingSize = (unsigned short)buffers[2].cbBuffer;
        auto trailerSize = (unsigned short)buffers[0].cbBuffer;
        SecBufferSizes cipherTextSizes(dataSize, paddingSize, trailerSize);

        KMemCpySafe(
            sizesPtr,
            sizeof(cipherTextSizes),
            &cipherTextSizes,
            sizeof(cipherTextSizes));

        if (cipherTextSizes.paddingSize)
        {
            encrypted.append(padding, paddingSize);
        }

        encrypted.append(trailer.data(), trailerSize);
    }

    encrypted.SetSizeAfterAppend();
    return SEC_E_OK;
}

SECURITY_STATUS SecurityContextWin::DecryptMessage(bique<byte> & receiveQueue, bique<byte> & decrypted)
{
    auto inputIter = receiveQueue.begin();
    auto const inputEnd = receiveQueue.end();
    auto cipherTextLength = inputEnd - inputIter;

    //todo, leikong, avoid the following copying when possible
    ByteBuffer2 inputMerged(cipherTextLength);
    while (inputIter < inputEnd)
    {
        auto fragmentSize = inputIter.fragment_size();
        auto fragmentEnd = inputIter + fragmentSize;
        auto remaining = inputEnd - inputIter;
        auto chunkSize = (uint)((fragmentEnd <= inputEnd) ? fragmentSize : remaining);
        inputMerged.append(&(*inputIter), chunkSize);
        inputIter += chunkSize;
    }

    inputMerged.SetSizeAfterAppend();

    WriteNoise(TraceType, id_, "DecryptMessage: merged input: {0:l}", inputMerged);

    auto inputPtr = inputMerged.data();
    while(inputPtr < inputMerged.end())
    {
        cipherTextLength = inputMerged.end() - inputPtr;
        if (cipherTextLength > numeric_limits<uint>::max())
        {
            WriteError(TraceType, id_, "DecryptMessage: ciphertext too long: {0}", cipherTextLength);
            return STATUS_UNSUCCESSFUL;
        }

        if (cipherTextLength < sizeof(SecBufferSizes))
        {
            WriteNoise(
                TraceType, id_,
                "DecryptMessage: incomplete message, cipherTextLength = {0}, sizeof(SecBufferSizes) = {1}",
                cipherTextLength, sizeof(SecBufferSizes));

            receiveQueue.truncate_before(inputEnd - cipherTextLength);
            return STATUS_PENDING;
        }

        SecBufferSizes sizes;
        KMemCpySafe(&sizes, sizeof(sizes), inputPtr, sizeof(sizes));

        if (sizes.dataSize > maxEncryptSize_)
        {
            WriteError(TraceType, id_, "DecryptMessage: dataSize({0}) > maxEncryptSize_({1})", sizes.dataSize, maxEncryptSize_);
            return STATUS_DATA_ERROR;
        }

        auto neededForDecrypt = sizeof(SecBufferSizes) + sizes.dataSize + sizes.paddingSize + sizes.tokenSize;
        if ((uint)cipherTextLength < neededForDecrypt)
        {
            WriteNoise(
                TraceType, id_,
                "DecryptMessage: incomplete message, cipherTextLength = {0}, need {1}",
                cipherTextLength, neededForDecrypt);

            receiveQueue.truncate_before(inputEnd - cipherTextLength);
            return STATUS_PENDING;
        }

        inputPtr += sizeof(SecBufferSizes);
        auto dataPtr = inputPtr;
        inputPtr += sizes.dataSize;
        auto paddingPtr = inputPtr;
        inputPtr += sizes.paddingSize;
        auto trailerPtr = inputPtr;
        inputPtr += sizes.tokenSize;

        SecBuffer buffers[3];
        buffers[0].BufferType = SECBUFFER_TOKEN;
        buffers[0].pvBuffer = trailerPtr;
        buffers[0].cbBuffer = sizes.tokenSize;

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = dataPtr;
        buffers[1].cbBuffer = sizes.dataSize;

        SecBufferDesc secBufferDesc;
        secBufferDesc.ulVersion = SECBUFFER_VERSION;
        secBufferDesc.pBuffers = buffers;

        if (sizes.paddingSize)
        {
            buffers[2].BufferType = SECBUFFER_PADDING;
            buffers[2].pvBuffer = paddingPtr;
            buffers[2].cbBuffer = sizes.paddingSize;

            secBufferDesc.cBuffers = 3;
        }
        else
        {
            secBufferDesc.cBuffers = 2;
        }

#if DBG && 0
        WriteNoise(
            TraceType, id_,
            "DecryptMessage: DecryptMessage input: data: {0:l}, padding: {1:l}, trailer: {2:l}",
            ConstBuffer(dataPtr, sizes.dataSize), ConstBuffer(paddingPtr, sizes.paddingSize), ConstBuffer(trailerPtr, sizes.tokenSize));
#endif

        ULONG fQOP = 0;
        SECURITY_STATUS status = ::DecryptMessage(&hSecurityContext_, &secBufferDesc, 0, &fQOP);
        if (FAILED(status))
        {
            WriteError(TraceType, id_, "DecryptMessage failed: 0x{0:x}", (uint)status);
            return status;
        }

#if DBG && 0
        WriteNoise(TraceType, id_, "DecryptMessage: DecryptMessage output: data: {0:l}", ConstBuffer(dataPtr, sizes.dataSize));
#endif

        // todo, leikong, consider reducing copy, is it possible to do in-place-decryption most of time and only copy when required?
        decrypted.append(dataPtr, sizes.dataSize);
    }

    receiveQueue.truncate_before(inputEnd);
    return SEC_E_OK;
}

SECURITY_STATUS SecurityContextWin::SignMessage(TcpFrameHeader const & frameHeader, Message & message, ByteBuffer2 & signedMessage)
{
    auto encryptCount = (frameHeader.FrameLength() + maxEncryptSize_ - 1) / maxEncryptSize_;
    auto chunkOverhead = (uint)sizeof(SecBufferSizes) + sizes_.cbSecurityTrailer;
    auto outputLimit = frameHeader.FrameLength() + chunkOverhead * encryptCount;
    if (outputLimit > numeric_limits<uint>::max())
    {
        WriteError(TraceType, id_, "SignMessage: too much input: {0}", outputLimit);
        return E_FAIL;
    }

    signedMessage.resize(outputLimit);

    vector<MutableBuffer> chunks;
    chunks.reserve(64);
    chunks.emplace_back((void*)&frameHeader, sizeof(frameHeader));

    for (BiqueChunkIterator chunk = message.BeginHeaderChunks(); chunk != message.EndHeaderChunks(); ++chunk)
    {
        chunks.emplace_back((void*)chunk->cbegin(), chunk->size());
    }

    for (BufferIterator chunk = message.BeginBodyChunks(); chunk != message.EndBodyChunks(); ++chunk)
    {
        chunks.emplace_back((void*)chunk->cbegin(), chunk->size());
    }

    ByteBuffer2 signature(sizes_.cbMaxSignature);

    auto iter = chunks.cbegin();
    uint chunkOffset = 0;
    while (iter != chunks.cend())
    {
        auto sizesPtr = signedMessage.AppendCursor();
        auto dataPtr = signedMessage.AdvanceAppendCursor(sizeof(SecBufferSizes));

        uint dataSize = 0;
        do
        {
            auto chunkStart = iter->buf + chunkOffset;
            auto chunkSize = iter->len - chunkOffset;
            if ((dataSize + chunkSize) > (maxEncryptSize_ - chunkOverhead))
            {
                auto toCopy = maxEncryptSize_ - chunkOverhead - dataSize;
                signedMessage.append(chunkStart, toCopy);
                chunkOffset += toCopy;
                dataSize = maxEncryptSize_ - chunkOverhead;
                break;
            }

            signedMessage.append(chunkStart, chunkSize); // todo, leikong, consider avoiding copy by using a list of output buffers
            dataSize += chunkSize;
            ++iter;
            chunkOffset = 0;
        } while (iter != chunks.cend());

        SecBuffer buffers[2];
        buffers[0].BufferType = SECBUFFER_TOKEN;
        buffers[0].pvBuffer = signature.data();
        buffers[0].cbBuffer = (uint)signature.size();

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = dataPtr;
        buffers[1].cbBuffer = dataSize;

        SecBufferDesc secBufferDesc;
        secBufferDesc.ulVersion = SECBUFFER_VERSION;
        secBufferDesc.pBuffers = buffers;
        secBufferDesc.cBuffers = 2;

        SECURITY_STATUS status = ::MakeSignature(&hSecurityContext_, 0, &secBufferDesc, 0);
        if (FAILED(status))
        {
            WriteError(TraceType, id_, "SignMessage: MakeSignature failed: 0x{0:x}", (uint)status);
            return status;
        }

        SecBufferSizes sizes(dataSize, 0, (unsigned short)buffers[0].cbBuffer);

        KMemCpySafe(sizesPtr, sizeof(sizes), &sizes, sizeof(sizes));
        signedMessage.append(signature.data(), sizes.tokenSize);
    }

    signedMessage.SetSizeAfterAppend();
    return SEC_E_OK;
}

SECURITY_STATUS SecurityContextWin::VerifySignedMessage(Common::bique<byte> & receiveQueue, Common::bique<byte> & output)
{
    auto inputIter = receiveQueue.begin();
    auto const inputEnd = receiveQueue.end();
    auto remainingInput = inputEnd - inputIter;

    //todo, leikong, avoid copying for both merging and output
    ByteBuffer2 inputMerged(remainingInput);
    while (inputIter < inputEnd)
    {
        auto fragmentSize = inputIter.fragment_size();
        auto fragmentEnd = inputIter + fragmentSize;
        auto remaining = inputEnd - inputIter;
        auto chunkSize = (uint)((fragmentEnd <= inputEnd) ? fragmentSize : remaining);
        inputMerged.append(&(*inputIter), chunkSize);
        inputIter += chunkSize;
    }

    inputMerged.SetSizeAfterAppend();

    WriteNoise(TraceType, id_, "VerifySignedMessage: merged input: {0:l}", inputMerged);

    auto inputPtr = inputMerged.data();
    while (inputPtr < inputMerged.end())
    {
        remainingInput = inputMerged.end() - inputPtr;
        if (remainingInput > numeric_limits<uint>::max())
        {
            WriteError(TraceType, id_, "VerifySignedMessage: ciphertext too long: {0}", remainingInput);
            return STATUS_UNSUCCESSFUL;
        }

        if (remainingInput < sizeof(SecBufferSizes))
        {
            WriteNoise(
                TraceType, id_,
                "VerifySignedMessage: incomplete message, remainingInput = {0}, sizeof(SecBufferSizes) = {1}",
                remainingInput, sizeof(SecBufferSizes));

            receiveQueue.truncate_before(inputEnd - remainingInput);
            return STATUS_PENDING;
        }

        SecBufferSizes sizes;
        KMemCpySafe(&sizes, sizeof(sizes), inputPtr, sizeof(sizes));

        if (sizes.dataSize > maxEncryptSize_)
        {
            WriteError(TraceType, id_, "VerifySignedMessage: dataSize({0}) > maxEncryptSize_({1})", sizes.dataSize, maxEncryptSize_);
            return STATUS_DATA_ERROR;
        }

        auto neededForDecrypt = sizeof(SecBufferSizes) + sizes.dataSize + sizes.tokenSize;
        if ((uint)remainingInput < neededForDecrypt)
        {
            WriteNoise(
                TraceType, id_,
                "VerifySignedMessage: incomplete message, remainingInput = {0}, need {1}",
                remainingInput, neededForDecrypt);

            receiveQueue.truncate_before(inputEnd - remainingInput);
            return STATUS_PENDING;
        }

        inputPtr += sizeof(SecBufferSizes);
        auto dataPtr = inputPtr;
        inputPtr += sizes.dataSize;
        auto signaturePtr = inputPtr;
        inputPtr += sizes.tokenSize;

        SecBuffer buffers[2];
        buffers[0].BufferType = SECBUFFER_TOKEN;
        buffers[0].pvBuffer = signaturePtr;
        buffers[0].cbBuffer = sizes.tokenSize;

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = dataPtr;
        buffers[1].cbBuffer = sizes.dataSize;

        SecBufferDesc secBufferDesc;
        secBufferDesc.ulVersion = SECBUFFER_VERSION;
        secBufferDesc.pBuffers = buffers;
        secBufferDesc.cBuffers = 2;

#if DBG && 0
        WriteNoise(
            TraceType, id_,
            "VerifySignedMessage: DecryptMessage input: data: {0:l}, signature: {2:l}",
            ConstBuffer(dataPtr, sizes.dataSize), ConstBuffer(signaturePtr, sizes.tokenSize));
#endif

        ULONG fQOP = 0;
        auto status = ::VerifySignature(&hSecurityContext_, &secBufferDesc, 0, &fQOP);
        if (FAILED(status))
        {
            WriteWarning(TraceType, id_, "VerifySignedMessage: VerifySignature failed: 0x{0:x}", (uint)status);
            return status;
        }

#if DBG && 0
        WriteNoise(TraceType, id_, "VerifySignedMessage: DecryptMessage output: data: {0:l}", ConstBuffer(dataPtr, sizes.dataSize));
#endif

        // todo, leikong, consider avoiding copy
        output.append(dataPtr, sizes.dataSize);
    }

    receiveQueue.truncate_before(inputEnd);
    return SEC_E_OK;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::EncodeMessage(MessageUPtr & message)
{
    SECURITY_STATUS status = SEC_E_OK;

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::None
        && transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::None)
    {
        return status;
    }

    SecurityHeader header(transportSecurity_->MessageHeaderProtectionLevel, transportSecurity_->MessageBodyProtectionLevel);

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign)
    {
        status = CalculateSignature(message, true, header.HeaderToken);
    }
    else if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        // copy headers

        size_t total = 0;

        for (auto iter = message->BeginHeaderChunks(); iter != message->EndHeaderChunks(); ++iter)
        {
            total += iter->size();
        }

        header.HeaderData.resize(total);
        byte * pb = header.HeaderData.data();

        for (auto iter = message->BeginHeaderChunks(); iter != message->EndHeaderChunks(); ++iter)
        {
            size_t cb = iter->size();
            auto destLimit = header.HeaderData.data() + header.HeaderData.size() - pb;

            Invariant(destLimit >= 0);
            KMemCpySafe(pb, static_cast<size_t>(destLimit), iter->cbegin(), cb);
            pb += cb;
        }

        status = EncryptMessagePart(header.HeaderToken, header.HeaderData, header.HeaderPadding);

        // remove headers if no encryption of body (which would create a new message)
        if (transportSecurity_->MessageBodyProtectionLevel != ProtectionLevel::EncryptAndSign)
        {
            // Headers were already compacted so replacing the existing headers is sufficient
            message->Headers.ReplaceUnsafe(ByteBiqueRange(EmptyByteBique.begin(), EmptyByteBique.end(), false));
        }
    }

    if (FAILED(status))
    {
        return status;
    }

    if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::Sign)
    {
        status = CalculateSignature(message, false, header.BodyToken);
    }
    else if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        std::unique_ptr<std::vector<byte>> bodyData(new std::vector<byte>);
        bodyData->resize(message->SerializedBodySize());
        byte * pb = bodyData->data();

        for (auto chunk = message->BeginBodyChunks(); chunk != message->EndBodyChunks(); ++chunk)
        {
            size_t cb = chunk->size();
            auto destLimit = bodyData->data() + bodyData->size() - pb;

            Invariant(destLimit >= 0);
            KMemCpySafe(pb, static_cast<size_t>(destLimit), chunk->cbegin(), cb);
            pb += cb;
        }

        status = EncryptMessagePart(header.BodyToken, *bodyData, header.BodyPadding);

        if (SUCCEEDED(status))
        {
            std::vector<Common::const_buffer> bufferList;
            bufferList.push_back(Common::const_buffer(bodyData->data(), bodyData->size()));

            auto secureMessage = std::unique_ptr<Message>(new Message(bufferList, FreeEncryptedBuffer, bodyData.release()));

            if (transportSecurity_->MessageHeaderProtectionLevel != ProtectionLevel::EncryptAndSign)
            {
                // If headers are encrypted they will all be in the security header, otherwise we need to move them
                // to the secure message
                secureMessage->Headers.AppendFrom(message->Headers);
            }

            if (message->HasSendStatusCallback())
            {
                secureMessage->SetSendStatusCallback(message->get_SendStatusCallback());
            }

            secureMessage->SetTraceId(message->TraceId());
            secureMessage->MoveLocalTraceContextFrom(*message);
            message = std::move(secureMessage);
        }
    }

    if (FAILED(status))
    {
        return status;
    }

    message->Headers.Add<SecurityHeader>(header);

    return status;
}

SECURITY_STATUS SecurityContextWin::ProcessClaimsMessage(MessageUPtr &)
{
    return SEC_E_OK;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::DecodeMessage(MessageUPtr & message)
{
    SECURITY_STATUS status = SEC_E_OK;

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::None 
        && transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::None)
    {
        return status;
    }

    SecurityHeader header;
    int headerCount = 0;

    bool foundSecurityHeader = false;
    for (auto iter = message->Headers.Begin(); iter != message->Headers.End(); ++iter) 
    {
        ++headerCount;

        if (iter->Id() == MessageHeaderId::MessageSecurity)
        {
            header = iter->Deserialize<SecurityHeader>();
            if (!iter->IsValid)
            {
                WriteError(TraceType, id_, "failed to deserialize security header: {0}", iter->Status);
                return iter->Status;
            }

            message->Headers.Remove(iter);

            if (foundSecurityHeader)
            {
                WriteError(TraceType, id_, "We already have a security header");
                Assert::TestAssert();
                return E_FAIL;
            }

            foundSecurityHeader = true;
        }
    }

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        if (headerCount != 1)
        {
            WriteError(
                TraceType, id_, 
                "there should only be one header with encryption used: {0}, {1}",
                headerCount,
                *message);

            return E_FAIL;
        }
    }

    if (header.MessageHeaderProtectionLevel != transportSecurity_->MessageHeaderProtectionLevel
        && header.MessageBodyProtectionLevel != transportSecurity_->MessageBodyProtectionLevel)
    {
        // this also happens if we didn't find the header
        // TODO error code
        return E_FAIL;
    }

    message->Headers.Compact();

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign)
    {
        status = VerifySignature(message, true, header.HeaderToken);
    }
    else if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        status = DecryptMessageHeaders(header.HeaderToken, header.HeaderData, header.HeaderPadding);

        if (FAILED(status))
        {
            return status;
        }

        ByteBique decryptedBuffers(header.HeaderData.size());
        decryptedBuffers.append(header.HeaderData.begin(), header.HeaderData.size());

        ByteBiqueRange decryptedRange(decryptedBuffers.begin(), decryptedBuffers.end(), true);
        message->Headers.Replace(std::move(decryptedRange));
    }

    if (FAILED(status))
    {
        return status;
    }

    if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::Sign)
    {
        status = VerifySignature(message, false, header.BodyToken);
    }

    if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        // unlike encryption, we can decrypt in place in the receive buffer
        status = DecryptMessageBody(message, header.BodyToken, header.BodyPadding);
    }

    if (FAILED(status))
    {
        return status;
    }

    if (!CheckVerificationHeadersIfNeeded(*message)) return E_FAIL;

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::EncryptMessagePart(vector<byte> & token, vector<byte> & data, vector<byte> & padding)
{
    token.resize(sizes_.cbSecurityTrailer);
    padding.resize(sizes_.cbBlockSize);

    SecurityBuffers buffers;
    buffers.AddTokenBuffer(token);
    buffers.AddDataBuffer(data);

    if (padding.size() > 0)
    {
        buffers.AddPaddingBuffer(padding);
    }

    SECURITY_STATUS status = ::EncryptMessage(&hSecurityContext_, 0, buffers.GetBuffers(), 0);

    if (SUCCEEDED(status))
    {
        buffers.ResizeTokenBuffer(token);
        buffers.ResizePaddingBuffer(padding);
    }

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::DecryptMessageHeaders(vector<byte> & token, vector<byte> & data, vector<byte> & padding)
{
    SecurityBuffers buffers;
    ULONG fQOP = 0;

    buffers.AddTokenBuffer(token);
    buffers.AddDataBuffer(data);

    if (padding.size() > 0)
    {
        buffers.AddPaddingBuffer(padding);
    }

    SECURITY_STATUS status = ::DecryptMessage(&hSecurityContext_, buffers.GetBuffers(), 0, &fQOP);

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextWin::DecryptMessageBody(MessageUPtr & message, vector<byte> & token, vector<byte> & padding)
{
    SecurityBuffers buffers;
    ULONG fQOP = 0;

    buffers.AddTokenBuffer(token);

    for (auto chunk = message->BeginBodyChunks(); chunk != message->EndBodyChunks(); ++chunk)
    {
        buffers.AddDataBuffer(*chunk);
    }

    if (padding.size() > 0)
    {
        buffers.AddPaddingBuffer(padding);
    }

    auto buffersDesc = buffers.GetBuffers();

    SECURITY_STATUS status = ::DecryptMessage(&hSecurityContext_, buffersDesc, 0, &fQOP);
    if (SUCCEEDED(status))
    {
        // Find out the total length of decrypted data first, so that we can guarantee that the new message body bique has
        // a single chunk, thus only needs one memory allocation (bique constructor parameter specifies its chunk size).
        size_t totalDecryptedBytes = 0;
        for (ULONG i = 0; i < buffersDesc->cBuffers; ++ i)
        {
            if (buffersDesc->pBuffers[i].BufferType == SECBUFFER_DATA)
            {
                totalDecryptedBytes += buffersDesc->pBuffers[i].cbBuffer;
            }
        }

        ByteBique newBody(totalDecryptedBytes);
        for (ULONG i = 0; i < buffersDesc->cBuffers; ++ i)
        {
            if (buffersDesc->pBuffers[i].BufferType == SECBUFFER_DATA)
            {
                newBody.append(reinterpret_cast<byte*>(buffersDesc->pBuffers[i].pvBuffer), buffersDesc->pBuffers[i].cbBuffer);
            }
        }

        ByteBiqueRange range(std::move(newBody));
        auto newMessage = Common::make_unique<Message>(ByteBiqueRange(EmptyByteBique.begin(), EmptyByteBique.end(), false), std::move(range), message->ReceiveTime());
        newMessage->Headers.AppendFrom(message->Headers);
        message = std::move(newMessage);
        message->SetSecurityContext(this);
    }

    return status;
}

SECURITY_STATUS SecurityContextWin::AuthorizeRemoteEnd()
{
    if (inbound_)
    {
        return AuthorizeWindowsClient();
    }

    // NTLM does not verify server side
    return SEC_E_OK;
}

void SecurityContextWin::TryTraceRemoteGroupSidAndLocalSecuritySettings() const
{
    static ULONGLONG lastTraceTimeInMilliSeconds = 0;

    ULONGLONG now = KNt::GetTickCount64();
    if ((now - lastTraceTimeInMilliSeconds) < SecurityConfig::GetConfig().RemoteGroupSidTraceInterval.TotalPositiveMilliseconds())
    {
        return;
    }

    lastTraceTimeInMilliSeconds = now;

    DWORD length = 0;
    GetTokenInformation(remoteToken_, TokenGroups, nullptr, 0, &length);
    vector<byte> buffer(length);
    if (!GetTokenInformation(remoteToken_, TokenGroups, &buffer.front(), length, &length))
    {
        WriteWarning(TraceType, id_, "GetTokenInformation(TokenGroups) failed on remote token: {0}", ::GetLastError());
        return;
    }

    TOKEN_GROUPS & remoteGroupToken = reinterpret_cast<TOKEN_GROUPS&>(buffer.front());
    if (remoteGroupToken.GroupCount == 0) return;

    wstring groupSids;
    StringWriter w(groupSids);
    for (ULONG i = 0; i < remoteGroupToken.GroupCount; ++i)
    {
        wstring groupSid;
        auto error = Sid::ToString(remoteGroupToken.Groups[i].Sid, groupSid);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "Failed to convert remote group sid to string: {0}", error);
            return;
        }

        w.Write("({0},0x{1})", groupSid, remoteGroupToken.Groups[i].Attributes);
    }


    WriteInfo(TraceType, id_, "remote group sids = {0}", groupSids);
    WriteInfo(TraceType, id_, "local security settings: {0}", transportSecurity_->ToString());
}

void SecurityContextWin::TraceRemoteSid() const
{
    DWORD length = 0;
    GetTokenInformation(remoteToken_, TokenUser, nullptr, 0, &length);
    vector<byte> buffer(length);
    if (!GetTokenInformation(remoteToken_, TokenUser, &buffer.front(), length, &length))
    {
        WriteWarning(TraceType, id_, "GetTokenInformation(TokenUser) failed on remote token: {0}", ::GetLastError());
        return;
    }

    TOKEN_USER & remoteUserToken = reinterpret_cast<TOKEN_USER&>(buffer.front());
    wstring remoteSid;
    auto error = Sid::ToString(remoteUserToken.User.Sid, remoteSid);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Failed to convert remote sid to string: {0}", error);
        return;
    }

    WriteInfo(TraceType, id_, "remote sid = {0}", remoteSid);
}

SECURITY_STATUS SecurityContextWin::AuthorizeWindowsClient()
{
    SECURITY_STATUS status = ::QuerySecurityContextToken(&hSecurityContext_, &remoteToken_);
    if (FAILED(status))
    {
        WriteWarning(TraceType, id_, "QuerySecurityContextToken() failed with 0x{0:x}", (uint)status);
        return status;
    }

    TraceRemoteSid();

    if (AccessCheck(transportSecurity_->ConnectionAuthSecurityDescriptor(), FILE_GENERIC_EXECUTE, &genericMapping))
    {
        if (transportSecurity_->Settings().IsClientRoleInEffect())
        {
            if (AccessCheck(transportSecurity_->AdminRoleSecurityDescriptor(), FILE_GENERIC_EXECUTE, &genericMapping))
            {
                roleMask_ = RoleMask::Admin;
            }
        }

        WriteInfo(TraceType, id_, "IsClientRoleInEffect = {0}, RoleMask = {1}", transportSecurity_->Settings().IsClientRoleInEffect(), roleMask_);
        return SEC_E_OK;
    }

    WriteWarning(TraceType, id_, "connection request denied");
    TryTraceRemoteGroupSidAndLocalSecuritySettings();
    return E_FAIL;
}

SECURITY_STATUS SecurityContextWin::OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const & negotiationInfo)
{
    bool mutualAuth = (negotiationInfo.PackageInfo->fCapabilities & SECPKG_FLAG_MUTUAL_AUTH) != 0;
    if (!mutualAuth)
    {
        auto owner = connection_.lock();
        if (!owner)
        {
            WriteInfo(TraceType, id_, "connection is already disposed");
            return E_FAIL;
        }

        if (!owner->IsLoopback())
        {
            WriteWarning(TraceType, id_, "mutual authentication should be done for non-loopback communication");
            if (SecurityConfig::GetConfig().MutualAuthRequiredForWindowsSecurity)
            {
                return E_FAIL;
            }
        }
    }

    SECURITY_STATUS status = QueryAttributes(SECPKG_ATTR_SIZES, &sizes_);
    if (status != SEC_E_OK)
    {
        WriteWarning(
            TraceType, id_, 
            "OnNegotiationSucceeded: QueryAttributes(SECPKG_ATTR_SIZES) failed: 0x{0:x}",
            (uint)status);

        return status;
    }

    SecPkgContext_SessionKey sk = {};
    status = QueryAttributes(SECPKG_ATTR_SESSION_KEY, &sk);
    if (status != SEC_E_OK)
    {
        WriteWarning(
            TraceType, id_,
            "OnNegotiationSucceeded: QueryAttributes(SECPKG_ATTR_SESSION_KEY) failed: 0x{0:x}",
            (uint)status);
    }

    WriteInfo(
        TraceType, id_,
        "SECPKG_ATTR_SIZES: cbMaxToken = {0}, cbMaxSignature = {1}, cbBlockSize = {2}, cbSecurityTrailer = {3}; SessionKeySize: {4} bit",
        sizes_.cbMaxToken, sizes_.cbMaxSignature, sizes_.cbBlockSize, sizes_.cbSecurityTrailer, sk.SessionKeyLength*8);

    return status;
}

void SecurityContextWin::SignatureBuffers(
    SecurityBuffers & buffers,
    MessageUPtr const & message,
    bool headers,
    std::vector<byte> & signature)
{
    buffers.AddTokenBuffer(signature);
    if (!headers)
    {
        for (auto body = message->BeginBodyChunks(); body != message->EndBodyChunks(); ++body)
        {
            WSABUF buffer = static_cast<WSABUF>(*body);
            buffers.AddDataBuffer(buffer);
        }
        return;
    }

    // Need to skip deleted headers when collecting input data for generating signature
    // We cannot compact headers here because the message might have been cloned.

    auto currentHeader = message->Headers.Begin();
    auto currentChunk = message->BeginHeaderChunks();
    if ((currentHeader == message->Headers.End()) || (currentChunk == message->EndHeaderChunks()))
    {
        return;
    }

    ULONG bytesToAddFromCurrentHeader = 0;
    ULONG bytesToSkip = 0;
    ULONG bytesScannedInCurrentChunk = 0;

    // A buffer begins on a chunk beginning or the ending of skipped/deleted bytes that precede,
    // and it ends on the ending of the current chunk or the starting of skipped/deleted bytes
    WSABUF bufferToAdd = {0, currentChunk->buf};

    do
    {
        if (currentHeader->DeletedBytesBetweenThisAndPredecessor() > 0)
        {
            // None-zero skipped bytes indicates we have reached the end of a buffer,
            // so we need to add the buffer that has ended, and start a new one
            if (bufferToAdd.len > 0)
            {
                buffers.AddDataBuffer(bufferToAdd);
                bufferToAdd.buf =  nullptr;
                bufferToAdd.len = 0;
            }

            // Skip deleted headers, which might span multiple chunks
            bytesToSkip = (ULONG)currentHeader->DeletedBytesBetweenThisAndPredecessor();
            while (bytesToSkip >= (currentChunk->len - bytesScannedInCurrentChunk))
            {
                bytesToSkip -= (currentChunk->len - bytesScannedInCurrentChunk);
                ++ currentChunk;
                if (currentChunk == message->EndHeaderChunks())
                {
                    return;
                }

                bytesScannedInCurrentChunk = 0;
            }

            bytesScannedInCurrentChunk += bytesToSkip;
            bufferToAdd.buf = currentChunk->buf + bytesScannedInCurrentChunk;
            bufferToAdd.len = 0;
        }

        // Consume all bytes of the current header, which might span multiple chunks
        bytesToAddFromCurrentHeader = (ULONG)currentHeader->ByteTotalInStream();
        while (bytesToAddFromCurrentHeader >= (currentChunk->len - bytesScannedInCurrentChunk))
        {
            bufferToAdd.len += (currentChunk->len - bytesScannedInCurrentChunk);
            bytesToAddFromCurrentHeader -= (currentChunk->len - bytesScannedInCurrentChunk);
            buffers.AddDataBuffer(bufferToAdd);

            ++ currentChunk;
            if (currentChunk == message->EndHeaderChunks())
            {
                return;
            }

            bufferToAdd.buf = currentChunk->buf;
            bufferToAdd.len = 0;
            bytesScannedInCurrentChunk = 0;
        }

        if (bytesToAddFromCurrentHeader > 0)
        {
            bytesScannedInCurrentChunk += bytesToAddFromCurrentHeader;
            bufferToAdd.len += bytesToAddFromCurrentHeader;
            bytesToAddFromCurrentHeader = 0;
        }

        ++ currentHeader;
    } while (currentHeader != message->Headers.End());

    if (bufferToAdd.len > 0)
    {
        buffers.AddDataBuffer(bufferToAdd);
    }
}

SECURITY_STATUS SecurityContextWin::CalculateSignature(MessageUPtr const & message, bool headers, std::vector<byte> & signature)
{
    SecurityBuffers buffers;

    signature.resize(sizes_.cbMaxSignature);

    SignatureBuffers(buffers, message, headers, signature);

    SECURITY_STATUS status = ::MakeSignature(&hSecurityContext_, 0, buffers.GetBuffers(), 0);

    if (SUCCEEDED(status)) 
    {
        buffers.ResizeTokenBuffer(signature);
    }

    return status;
}

SECURITY_STATUS SecurityContextWin::VerifySignature(MessageUPtr const & message, bool headers, std::vector<byte> & signature)
{
    SecurityBuffers buffers;
    SignatureBuffers(buffers, message, headers, signature);

    ULONG fQOP = 0;
    return ::VerifySignature(&hSecurityContext_, buffers.GetBuffers(), 0, &fQOP);
}

bool SecurityContextWin::AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const
{
    return acl.AccessCheckWindows(remoteToken_, desiredAccess);
}

_Use_decl_annotations_
bool SecurityContextWin::AccessCheck(
    PSECURITY_DESCRIPTOR securityDescriptor,
    DWORD desiredAccess,
    PGENERIC_MAPPING mapping) const
{
    MapGenericMask(&desiredAccess, mapping);

    PRIVILEGE_SET privilegeSet;
    DWORD privSetSize = sizeof(PRIVILEGE_SET);
    DWORD grantedAccess;
    BOOL accessStatus;

    BOOL callSucceeded = ::AccessCheck(
        securityDescriptor,
        remoteToken_,
        desiredAccess,
        mapping,
        &privilegeSet,
        &privSetSize,
        &grantedAccess,
        &accessStatus);

    if (callSucceeded == FALSE)
    {
        WriteError(TraceType, id_, "AccessCheck call failed: {0}", ::GetLastError());
        return false;
    }

    if (accessStatus == FALSE)
    {
        WriteInfo(TraceType, id_, "AccessCheck: output assessStatus = FALSE: {0}", ::GetLastError());
        return false;
    }

    bool accessCheckPassed = (grantedAccess == desiredAccess);
    if (!accessCheckPassed)
    {
        WriteInfo(TraceType, id_, "AccessCheck: different access granted: desired = {0}, granted = {1}", desiredAccess, grantedAccess);
    }

    return accessCheckPassed;
}

void SecurityContextWin::CompleteClientAuth(ErrorCode const &, SecuritySettings::RoleClaims const &, Common::TimeSpan)
{
    Assert::CodingError("Not implemented");
}

bool SecurityContextWin::Supports(SecurityProvider::Enum provider)
{
    return provider == SecurityProvider::Kerberos|| provider == SecurityProvider::Negotiate;
}
