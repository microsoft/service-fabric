// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpGateway;
using namespace Serialization;
using namespace Management::CentralSecretService;

namespace JsonObjectModels {
    class SecretPropertiesDescription
        : public IFabricJsonSerializable
    {
    public:
        SecretPropertiesDescription()
            : kind_()
            , description_()
            , contentType_()
        {
        }

        SecretPropertiesDescription(Secret const & secret)
            : kind_(secret.Kind)
            , description_(secret.Description)
            , contentType_(secret.ContentType)
        {
        }

        __declspec (property(get = get_Kind)) wstring const & Kind;
        wstring const & get_Kind() const { return kind_; }

        __declspec (property(get = get_ContentType)) wstring const & ContentType;
        wstring const & get_ContentType() const { return contentType_; }

        __declspec (property(get = get_Description)) wstring const & Description;
        wstring const & get_Description() const { return description_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"kind", kind_)
            SERIALIZABLE_PROPERTY(L"description", description_)
            SERIALIZABLE_PROPERTY(L"contentType", contentType_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring kind_;
        wstring description_;
        wstring contentType_;
    };

    class SecretValuePropertiesDescription
        : public IFabricJsonSerializable
    {
    public:
        SecretValuePropertiesDescription()
            : value_()
        {
        }

        SecretValuePropertiesDescription(Secret const & secret)
            : value_(secret.Value)
        {
        }

        virtual ~SecretValuePropertiesDescription()
        {
            SecureZeroMemory((void *)this->value_.c_str(), this->value_.size() * sizeof(wchar_t));
        }

        __declspec (property(get = get_Value)) wstring const & Value;
        wstring const & get_Value() const { return value_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"value", value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring value_;
    };

    class SecretDescription
        : public IFabricJsonSerializable
    {
    public:
        SecretDescription()
            : name_()
            , properties_()
        {
        }

        SecretDescription(Secret const & secret)
            : name_(secret.Name)
            , properties_(secret)
        {
        }

        virtual ~SecretDescription() {
        }

        __declspec (property(get = get_Name, put = set_Name)) wstring const & Name;
        wstring const & get_Name() const { return name_; }
        void set_Name(std::wstring const & name) { name_ = name; }

        __declspec (property(get = get_Properties)) SecretPropertiesDescription const & Properties;
        SecretPropertiesDescription const & get_Properties() const { return properties_; }

        Secret ToSecret() const
        {
            return Secret(
                this->name_,
                L"",            // version
                L"",            // value
                this->properties_.Kind,
                this->properties_.ContentType,
                this->properties_.Description);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"name", name_)
            SERIALIZABLE_PROPERTY(L"properties", properties_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring name_;
        SecretPropertiesDescription properties_;
    };

    class SecretValueDescription
        : public IFabricJsonSerializable
    {
    public:
        SecretValueDescription()
            : version_()
            , properties_()
        { }

        __declspec (property(get = get_Version, put = set_Version)) wstring const & Version;
        wstring const & get_Version() const { return version_; }
        void set_Version(wstring const & version) { version_ = version; }

        __declspec (property(get = get_Properties)) SecretValuePropertiesDescription const & Properties;
        SecretValuePropertiesDescription const & get_Properties() const { return properties_; }

        SecretValueDescription(Secret const & secret)
            : version_(secret.Version)
            , properties_(secret)
        {
        }

        SecretValueDescription(SecretReference const & secretReference)
            : version_(secretReference.Version)
            , properties_()
        {
        }

        Secret ToSecret(wstring const & name) const
        {
            return Secret(
                name,
                this->version_,
                this->properties_.Value,
                L"",            // kind
                L"",            // content type
                L"");           // description
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"properties", properties_)
            SERIALIZABLE_PROPERTY(L"name", version_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring version_;
        SecretValuePropertiesDescription properties_;
    };

    ///
    /// Class representing the value of a secret version as a raw string, as opposed to a proper resource.
    ///
    class SecretValue
        : public IFabricJsonSerializable
    {
    public:
        SecretValue()
            : value_()
        { }

        SecretValue(Secret const & secret)
            : value_(secret.Value)
        {
        }

        virtual ~SecretValue()
        {
            SecureZeroMemory((void *)this->value_.c_str(), this->value_.size() * sizeof(wchar_t));
        }

        __declspec (property(get = get_Value, put = set_Value)) wstring const & Value;
        wstring const & get_Value() const { return value_; }
        void set_value(std::wstring const & value) { this->value_ = value; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"value", value_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring value_;
    };

    class PagedSecretDescriptionCollection
        : public IFabricJsonSerializable
    {
    public:
        PagedSecretDescriptionCollection()
            : continuationToken_()
            , secrets_()
        {
        }

        PagedSecretDescriptionCollection(vector<Secret> const & secrets, wstring const & continuationToken)
            : continuationToken_(continuationToken)
            , secrets_()
        {
            transform(
                secrets.begin(),
                secrets.end(),
                back_inserter(this->secrets_),
                [](Secret const & secret)
                {
                    return SecretDescription(secret);
                });
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ContinuationToken", continuationToken_)
            SERIALIZABLE_PROPERTY(L"Items", secrets_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring continuationToken_;
        vector<SecretDescription> secrets_;
    };

    class PagedSecretValueDescriptionCollection
        : public IFabricJsonSerializable
    {
    public:
        PagedSecretValueDescriptionCollection()
            : continuationToken_()
            , secretValues_()
        {
        }

        PagedSecretValueDescriptionCollection(vector<Secret> const & secrets, wstring const & continuationToken)
            : continuationToken_(continuationToken)
            , secretValues_()
        {
            transform(
                secrets.begin(),
                secrets.end(),
                back_inserter(this->secretValues_),
                [](Secret const & secret)
            {
                return SecretValueDescription(secret);
            });
        }

        PagedSecretValueDescriptionCollection(vector<SecretReference> const & secretReferences, wstring const & continuationToken)
            : continuationToken_(continuationToken)
            , secretValues_()
        {
            transform(
                secretReferences.begin(),
                secretReferences.end(),
                back_inserter(this->secretValues_),
                [](SecretReference const & secretReference)
            {
                return SecretValueDescription(secretReference);
            });
        }


        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"ContinuationToken", continuationToken_)
            SERIALIZABLE_PROPERTY(L"Items", secretValues_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        wstring continuationToken_;
        vector<SecretValueDescription> secretValues_;
    };
}

void SecretsResourceHandler::HandleError(HandlerAsyncOperation* const & handlerOperation, AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusNotFound, Constants::StatusDescriptionNotFound);
    }
    else if (error.IsError(ErrorCodeValue::InvalidOperation))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusConflict, Constants::StatusDescriptionConflict);
    }
    else if (error.IsError(ErrorCodeValue::PropertyTooLarge)
        || error.IsError(ErrorCodeValue::SecretInvalid))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
    }
    else
    {
        handlerOperation->OnError(thisSPtr, error);
    }
}

SecretsResourceHandler::SecretsResourceHandler(HttpGatewayImpl &server)
    : RequestHandlerBase(server)
{
}

SecretsResourceHandler::~SecretsResourceHandler()
{
}

ErrorCode SecretsResourceHandler::Initialize()
{
    this->allowHierarchicalEntityKeys = false;

    // GET: /Resources/Secrets/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetSecrets)));

    // GET: /Resources/Secrets/{SecretName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetSecret)));

    // PUT: /Resources/Secrets/{SecretName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(PutSecret)));

    // DELETE: /Resources/Secrets/{SecretName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteSecret)));

    // GET: /Resources/Secrets/{SecretName}/values/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceVersionEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetSecretVersions)));

    // get the description of a versioned secret value
    // GET: /Resources/Secrets/{SecretName}/values/{SecretVersion}
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceVersionEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetSecretVersion)));

    // retrieve the value of the version of a secret
    // POST: /Resources/Secrets/{SecretName}/values/{SecretVersion}/list_value
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceVersionEntityKeyValuePath,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(GetVersionedSecretValue)));

    // PUT: /Resources/Secrets/{SecretName}/values/{SecretVersion}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceVersionEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(PutVersionedSecretValue)));

    // DELETE: /Resources/Secrets/{SecretName}/values/{SecretVersion}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::SecretsResourceVersionEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteSecretVersion)));

    return server_.InnerServer->RegisterHandler(Constants::SecretsResourceHandlerPath, shared_from_this());
}

/* GetSecrets */

void SecretsResourceHandler::GetSecrets(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    GetSecretsDescription description({ SecretReference() }, false);

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginGetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnGetSecretsComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnGetSecretsComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnGetSecretsComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndGetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    JsonObjectModels::PagedSecretDescriptionCollection pagedSecretCollection(description.Secrets, L"");

    error = handlerOperation->Serialize(pagedSecretCollection, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* GetSecret */

void SecretsResourceHandler::GetSecret(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    wstring secretName;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    GetSecretsDescription description({ SecretReference(secretName) }, false);

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginGetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnGetSecretComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnGetSecretComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnGetSecretComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndGetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    JsonObjectModels::SecretDescription secret(description.Secrets.at(0));

    error = handlerOperation->Serialize(secret, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* PutSecret */

void SecretsResourceHandler::PutSecret(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    wstring secretName;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    JsonObjectModels::SecretDescription secret;

    error = handlerOperation->Deserialize(secret, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    // The 'Name' specified in the request body, if provided, must be the same as the name specified in the URI.
    if (secret.Name.empty())
    {
        secret.Name = secretName;
    }
    else if (0 != secretName.compare(secret.Name))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    SecretsDescription description({ secret.ToSecret() });

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginSetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnPutSecretComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnPutSecretComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnPutSecretComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndSetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    JsonObjectModels::SecretDescription secret(description.Secrets.at(0));

    error = handlerOperation->Serialize(secret, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusCreated, Constants::StatusDescriptionCreated);
}

/* DeleteSecret */

void SecretsResourceHandler::DeleteSecret(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    wstring secretName;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    SecretReferencesDescription description({ SecretReference(secretName) });

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginRemoveSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnDeleteSecretComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnDeleteSecretComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnDeleteSecretComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretReferencesDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndRemoveSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* GetSecretVersions */

void SecretsResourceHandler::GetSecretVersions(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    wstring secretName;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    SecretReferencesDescription description({ SecretReference(secretName) });

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginGetSecretVersions(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnGetSecretVersionsComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnGetSecretVersionsComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnGetSecretVersionsComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretReferencesDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndGetSecretVersions(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    // Listing secret versions actually returns a wire/public API type of VersionedSecretValueDescription, but never
    // the secret values themselves; hence we convert a page of secret references (the output of the inner API) to
    // a page of values.
    ByteBufferUPtr body;
    JsonObjectModels::PagedSecretValueDescriptionCollection pagedSecretValueCollection(description.SecretReferences, L"");

    error = handlerOperation->Serialize(pagedSecretValueCollection, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* GetSecretVersion */

void SecretsResourceHandler::GetSecretVersion(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    wstring secretName;
    wstring secretVersion;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    if (!handlerOperation->Uri.GetItem(Constants::SecretVersionString, secretVersion))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    GetSecretsDescription description({ SecretReference(secretName, secretVersion) }, false);

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginGetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
    {
        this->OnGetSecretVersionComplete(operationSPtr, false);
    },
        thisSPtr);

    this->OnGetSecretVersionComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnGetSecretVersionComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndGetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    // TODO [dragosav]: for the time being, we're using the same service API as for retrieving
    // the secret's value, and will strip the value before deserializing into the wire object.
    // Post Ignite, revisit this and split retrieval of a version from retrieval of a value in 
    // the CSS service.
    // To strip the value, convert the Secret to a SecretReference, which invokes the 'empty'
    // SecretValueDescriptionProperties ctor.
    auto secret = description.Secrets.at(0);
    ByteBufferUPtr body;
    JsonObjectModels::SecretValueDescription secretVersion(SecretReference(secret.Name, secret.Version));

    error = handlerOperation->Serialize(secretVersion, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* GetVersionedSecretValue */

void SecretsResourceHandler::GetVersionedSecretValue(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    wstring secretName;
    wstring secretVersion;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    if (!handlerOperation->Uri.GetItem(Constants::SecretVersionString, secretVersion))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    GetSecretsDescription description({ SecretReference(secretName, secretVersion) }, true);

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginGetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnGetVersionedSecretValueComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnGetVersionedSecretValueComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnGetVersionedSecretValueComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    // TODO [dragosav]: for the time being, we're using the same service API as for retrieving
    // the secret's value, and will strip the value before deserializing into the wire object.
    // Post Ignite, revisit this and split retrieval of a version from retrieval of a value in 
    // the CSS service.
    // In this case, the retrieved secret includes the value, and will be serialized into a plain
    // SecretValue object.
    error = handlerOperation->FabricClient.SecretStoreClient->EndGetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    JsonObjectModels::SecretValue secretValue(description.Secrets.at(0));

    error = handlerOperation->Serialize(secretValue, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* PutVersionedSecretValue */

void SecretsResourceHandler::PutVersionedSecretValue(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    ErrorCode error;
    wstring secretName;
    wstring secretVersion;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    if (!handlerOperation->Uri.GetItem(Constants::SecretVersionString, secretVersion))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    JsonObjectModels::SecretValueDescription secretValue;

    error = handlerOperation->Deserialize(secretValue, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    // The 'Name' specified in the request body, if provided, must be the same as the secret version id specified in the URI.
    if (secretValue.Version.empty())
    {
        secretValue.Version = secretVersion;
    }
    else if (0 != secretVersion.compare(secretValue.Version))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    SecretsDescription description({ secretValue.ToSecret(secretName) });

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginSetSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnPutVersionedSecretValueComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnPutVersionedSecretValueComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnPutVersionedSecretValueComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretsDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndSetSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    JsonObjectModels::SecretValueDescription secretValue(description.Secrets.at(0));

    error = handlerOperation->Serialize(secretValue, body);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}

/* DeleteSecretVersion */

void SecretsResourceHandler::DeleteSecretVersion(AsyncOperationSPtr const &thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    wstring secretName;
    wstring secretVersion;

    if (!handlerOperation->Uri.GetItem(Constants::SecretNameString, secretName))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    if (!handlerOperation->Uri.GetItem(Constants::SecretVersionString, secretVersion))
    {
        handlerOperation->OnError(thisSPtr, Constants::StatusBadRequest, Constants::StatusDescriptionBadRequest);
        return;
    }

    SecretReferencesDescription description({ SecretReference(secretName, secretVersion) });

    auto secretsOperation = handlerOperation->FabricClient.SecretStoreClient->BeginRemoveSecrets(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operationSPtr)
        {
            this->OnDeleteSecretVersionComplete(operationSPtr, false);
        },
        thisSPtr);

    this->OnDeleteSecretVersionComplete(secretsOperation, true);
}

void SecretsResourceHandler::OnDeleteSecretVersionComplete(AsyncOperationSPtr const &operationSPtr, bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operationSPtr->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    ErrorCode error;
    SecretReferencesDescription description;

    error = handlerOperation->FabricClient.SecretStoreClient->EndRemoveSecrets(operationSPtr, description);
    if (!error.IsSuccess())
    {
        HandleError(handlerOperation, thisSPtr, error);
        return;
    }

    ByteBufferUPtr body;
    handlerOperation->OnSuccess(thisSPtr, move(body), Constants::StatusOk, Constants::StatusDescriptionOk);
}