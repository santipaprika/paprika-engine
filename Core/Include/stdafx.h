//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently.

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <wrl.h>

#include <filesystem>
#include <string>
#include <shellapi.h>

#include <memory>
#include <vector>

#include <Logger.h>

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

namespace PPK
{
    inline std::filesystem::path GetAssetsFilesystemPath()
    {
        return { ASSETS_PATH"/" };
    }

    inline std::wstring GetAssetsPath()
    {
        return GetAssetsFilesystemPath().wstring();
    }

    inline std::filesystem::path GetAssetFullFilesystemPath(LPCWSTR assetName)
    {
        const std::filesystem::path assetsFilesystemPath = GetAssetsFilesystemPath();
        return assetsFilesystemPath / assetName;
    }

    inline std::filesystem::path GetAssetFullFilesystemPath(std::string assetName)
    {
        const std::filesystem::path assetsFilesystemPath = GetAssetsFilesystemPath();
        return assetsFilesystemPath / assetName;
    }

    // Helper function for resolving the full path of assets.
    inline std::wstring GetAssetFullPath(LPCWSTR assetName)
    {
        return GetAssetFullFilesystemPath(assetName).wstring();
    }

    inline std::wstring GetAssetFullPath(std::string assetName)
    {
        return GetAssetFullFilesystemPath(assetName).wstring();
    }
}