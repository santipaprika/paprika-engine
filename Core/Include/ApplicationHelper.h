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

#pragma once
#include <stdafx.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXTex.h>
#include <WinPixEventRuntime/pix3.h>
#include <shlobj.h>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
using Microsoft::WRL::ComPtr;

inline std::wstring StringToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

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

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline void ThrowIfFailed(HRESULT hr, ID3DBlob* errors)
{
    if (errors)
    {
        void* blobData = errors->GetBufferPointer();
        OutputDebugStringA(static_cast<char*>(blobData));
    }

    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline HRESULT ReadDataFromFile(LPCWSTR filename, char** data, UINT* size)
{
    using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
    Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<char*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}

inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, char** data, UINT* offset, UINT* size)
{
    if (FAILED(ReadDataFromFile(filename, data, size)))
    {
        return E_FAIL;
    }

    // DDS files always start with the same magic number.
    static const UINT DDS_MAGIC = 0x20534444;
    UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
    if (magicNumber != DDS_MAGIC)
    {
        return E_FAIL;
    }

    struct DDS_PIXELFORMAT
    {
        UINT size;
        UINT flags;
        UINT fourCC;
        UINT rgbBitCount;
        UINT rBitMask;
        UINT gBitMask;
        UINT bBitMask;
        UINT aBitMask;
    };

    struct DDS_HEADER
    {
        UINT size;
        UINT flags;
        UINT height;
        UINT width;
        UINT pitchOrLinearSize;
        UINT depth;
        UINT mipMapCount;
        UINT reserved1[11];
        DDS_PIXELFORMAT ddsPixelFormat;
        UINT caps;
        UINT caps2;
        UINT caps3;
        UINT caps4;
        UINT reserved2;
    };

    auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
    if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT))
    {
        return E_FAIL;
    }

    const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
    *offset = ddsDataOffset;
    *size = *size - ddsDataOffset;

    return S_OK;
}

inline std::string HumanReadableSize(size_t bytes) {
    const char* units[] = { "B", "KB", "MB", "GB" };
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size > 1024 && unit < 3)
    {
        size /= 1024;
        ++unit;
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    return buffer;
}

// Assign a name to the object to aid with debugging.
inline void SetD3D12ObjectName(ID3D12Object* pObject, LPCWSTR name)
{
#ifdef PPK_DEBUG_GPU_RESOURCES
    pObject->SetName(name);
#endif
}

inline void SetD3D12ObjectName(ID3D12Object* pObject, LPCSTR name)
{
#ifdef PPK_DEBUG_GPU_RESOURCES
    SetD3D12ObjectName(pObject, StringToWstring(name).c_str());
#endif
}

inline void SetD3D12ObjectNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
#ifdef PPK_DEBUG_GPU_RESOURCES
    WCHAR fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        SetD3D12ObjectName(pObject, fullName);
    }
#endif
}

inline void SetD3D12ObjectNameIndexed(ID3D12Object* pObject, LPCSTR name, UINT index)
{
#ifdef PPK_DEBUG_GPU_RESOURCES
    SetD3D12ObjectNameIndexed(pObject, StringToWstring(name).c_str(), index);
#endif
}

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetD3D12ObjectName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_CUSTOM(x, name) SetD3D12ObjectName((x).Get(), name)
#define NAME_D3D12_OBJECT_NUMBERED_CUSTOM(x, name, i) SetD3D12ObjectNameIndexed((x).Get(), name, i)

inline const wchar_t* HeapTypeToString(D3D12_DESCRIPTOR_HEAP_TYPE heapType) {
    switch (heapType) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: return L"CBV_SRV_UAV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:     return L"SAMPLER";
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:         return L"RTV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:         return L"DSV";
        default: return L"INVALID_HEAP_TYPE";
    }
}

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    // Constant buffer size is required to be aligned.
    return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    PPK::Logger::Assert(false, "Attempting to compile shader with FXC, please use Renderer::CompileShader instead for DXC");

    UINT compileFlags = 0;
#ifdef PPK_PROFILE_SHADERS
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr;

    Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return byteCode;
}
#endif

// Resets all elements in a ComPtr array.
template<class T>
void ResetComPtrArray(T* comPtrArray)
{
    for (auto& i : *comPtrArray)
    {
        i.Reset();
    }
}


// Resets all elements in a unique_ptr array.
template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
    for (auto& i : *uniquePtrArray)
    {
        i.reset();
    }
}

#ifdef PPK_USE_PIX
inline std::wstring GetLatestWinPixGpuCapturerPath()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::wstring newestVersionFound;

    for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
    {
        if (directory_entry.is_directory())
        {
            if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
            {
                newestVersionFound = directory_entry.path().filename().c_str();
            }
        }
    }

    if (newestVersionFound.empty())
    {
        PPK::Logger::Error("Pix installation not found");
    }

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}
#endif

inline DirectX::ScratchImage LoadTextureFromDisk(const std::wstring& filePath)
{
    DirectX::ScratchImage image;

    // TODO: This is very slow, figure out why. For now it is parallelized to amortize cost.
    HRESULT hr = DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to load texture from disk");
    }
    return image;
}