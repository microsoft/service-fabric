//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "api/definitions/ApiDefinitions.h"
#include "systemservices/SystemServices.Service.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"
#include "Management/ResourceManager/ResourceManager.h"

#include "Management/CentralSecretService/Constants.h"
#include "Management/CentralSecretService/Crypto.h"
#include "Management/CentralSecretService/SecretDataItem.h"
#include "Management/CentralSecretService/SecretManager.h"
#include "Management/CentralSecretService/CentralSecretServicePointers.h"
#include "Management/CentralSecretService/CentralSecretServiceConfig.h"
#include "Management/CentralSecretService/ClientRequestAsyncOperation.h"
#include "Management/CentralSecretService/GetSecretsAsyncOperation.h"
#include "Management/CentralSecretService/SetSecretsAsyncOperation.h"
#include "Management/CentralSecretService/RemoveSecretsAsyncOperation.h"
#include "Management/CentralSecretService/CentralSecretServiceReplica.h"
#include "Management/CentralSecretService/CentralSecretServiceFactory.h"
