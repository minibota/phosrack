// SPDX-License-Identifier: GPL-3.0-or-later
#include "tokenstore.h"
#include <QRegularExpression>
#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincred.h>
#include <string>
#endif

namespace Phosrack::NativeTokenStore {
QString credentialTarget(const QString &clientId) {
    static const QRegularExpression valid("^[a-fA-F0-9]{32}$");
    return valid.match(clientId).hasMatch() ? "Phosrack/Spotify/" + clientId : QString();
}

ReadResult readRefreshToken(const QString &clientId) {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return {ReadStatus::Missing,{}};
    const auto key=credentialTarget(clientId);
    if(key.isEmpty())return {};
#ifdef Q_OS_WIN
    const std::wstring target=key.toStdWString();
    PCREDENTIALW credential=nullptr;
    if(!CredReadW(target.c_str(),CRED_TYPE_GENERIC,0,&credential))
        return {GetLastError()==ERROR_NOT_FOUND?ReadStatus::Missing:ReadStatus::Unavailable,{}};
    ReadResult result;
    if(credential->CredentialBlob && credential->CredentialBlobSize>0 &&
       credential->CredentialBlobSize<=CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        result={ReadStatus::Found,QByteArray(reinterpret_cast<const char*>(credential->CredentialBlob),
            int(credential->CredentialBlobSize))};
        SecureZeroMemory(credential->CredentialBlob,credential->CredentialBlobSize);
    } else if(credential->CredentialBlobSize==0)result.status=ReadStatus::Missing;
    CredFree(credential);
    return result;
#else
    return {};
#endif
}

bool writeRefreshToken(const QString &clientId,const QByteArray &token) {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return true;
    const auto key=credentialTarget(clientId);
    if(key.isEmpty()||token.isEmpty())return false;
#ifdef Q_OS_WIN
    if(token.size()>CRED_MAX_CREDENTIAL_BLOB_SIZE)return false;
    std::wstring target=key.toStdWString();
    wchar_t username[]=L"Spotify";
    QByteArray blob=token;
    CREDENTIALW credential{};
    credential.Type=CRED_TYPE_GENERIC;
    credential.TargetName=target.data();
    credential.UserName=username;
    credential.CredentialBlobSize=DWORD(blob.size());
    credential.CredentialBlob=reinterpret_cast<LPBYTE>(blob.data());
    // Persists across logons for this Windows user on this computer, not other users.
    credential.Persist=CRED_PERSIST_LOCAL_MACHINE;
    const bool written=CredWriteW(&credential,0)!=FALSE;
    SecureZeroMemory(blob.data(),SIZE_T(blob.size()));
    return written;
#else
    return false;
#endif
}

bool removeRefreshToken(const QString &clientId) {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return true;
    const auto key=credentialTarget(clientId);
    if(key.isEmpty())return false;
#ifdef Q_OS_WIN
    const std::wstring target=key.toStdWString();
    return CredDeleteW(target.c_str(),CRED_TYPE_GENERIC,0)!=FALSE || GetLastError()==ERROR_NOT_FOUND;
#else
    return false;
#endif
}
}
