/**
 * AnshuBio Unlock - Credential Provider GUIDs
 * Publisher: AnshuCore
 * App ID: com.anshucore.bio
 */

#pragma once
#include <initguid.h>

// {B36E9B9A-5827-463F-8C37-67AB12E09B10}
DEFINE_GUID(CLSID_AnshuBioCredentialProvider,
    0xb36e9b9a, 0x5827, 0x463f, 0x8c, 0x37, 0x67, 0xab, 0x12, 0xe0, 0x9b, 0x10);

// Credential Provider Name
#define ANSHUBIO_CP_NAME L"AnshuBio Unlock Credential Provider"
#define ANSHUBIO_PIPE_NAME L"\\\\.\\pipe\\AnshuBioUnlockAuthPipe"
