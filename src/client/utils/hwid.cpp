#include <std_include.hpp>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <comdef.h>
#include <Wbemidl.h>
#include <VersionHelpers.h>
#include "component/discord.hpp"
#include "tcp/hmw_tcp_utils.hpp"
#include "curl/curl.h"
#include "component/patches.hpp"
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "Wintrust.lib")
#pragma comment(lib, "Version.lib")
#define UNLEN 256

std::string simple_hash(const std::string& input) {
    unsigned long hash1 = 5381;
    unsigned long hash2 = 0;
    unsigned long hash3 = 0;
    int c;

    for (size_t i = 0; i < input.size(); ++i) {
        c = input[i];
        hash1 = ((hash1 << 5) + hash1) + c;
        hash2 = ((hash2 << 7) + hash2) + c;
        hash3 = ((hash3 << 3) + hash3) + c;
    }

    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << hash1;
    oss << std::hex << std::setw(8) << std::setfill('0') << hash2;
    oss << std::hex << std::setw(8) << std::setfill('0') << hash3;

    return oss.str();
}

std::string GetBuildConfiguration() {
#ifdef _DEBUG
    return "Debug";
#else
    return "Release";
#endif
}

#pragma region getHWID
std::wstring getUserName() {
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len)) {
        return std::wstring(username);
    }
    else {
        return L"";
    }
}

std::vector<std::string> GetAllDriveSerials() {
    std::vector<std::string> serialNumbers;
    DWORD drives = GetLogicalDrives();
    if (drives == 0) {
        return serialNumbers;
    }

    char driveLetter[4] = "A:\\";
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            driveLetter[0] = 'A' + i;

            UINT driveType = GetDriveTypeA(driveLetter);
            if (driveType != DRIVE_FIXED) {
                continue;
            }

            DWORD serialNumber = 0;
            if (GetVolumeInformationA(driveLetter, NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
                std::ostringstream ss;
                ss << serialNumber;
                std::string serialStr = ss.str();
                if (serialStr.length() > 3 && serialStr.find(' ') == std::string::npos) {
                    serialNumbers.push_back(serialStr);
                }
            }
        }
    }

    return serialNumbers;
}



std::string CombineSerialNumbers(const std::vector<std::string>& serialNumbers) {
    std::string combined;
    for (const auto& serial : serialNumbers) {
        if (!combined.empty()) {
            combined += "|";
        }
        combined += serial;
    }
    return combined;
}

std::string GetMotherboardSerialNumber() {
    std::string serialNumber = "";
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    IEnumWbemClassObject* enumerator = nullptr;

    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return serialNumber;
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return serialNumber;
    }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
    if (FAILED(hres)) {
        CoUninitialize();
        return serialNumber;
    }

    hres = locator->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &services);
    if (FAILED(hres)) {
        locator->Release();
        CoUninitialize();
        return serialNumber;
    }

    hres = services->ExecQuery(bstr_t(L"WQL"), bstr_t(L"SELECT SerialNumber FROM Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    if (FAILED(hres)) {
        services->Release();
        locator->Release();
        CoUninitialize();
        return serialNumber;
    }

    IWbemClassObject* clsObj = nullptr;
    ULONG uReturn = 0;
    while (enumerator) {
        HRESULT hr = enumerator->Next(WBEM_INFINITE, 1, &clsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        hr = clsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr) {
            serialNumber = _bstr_t(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
        clsObj->Release();
    }

    services->Release();
    locator->Release();
    enumerator->Release();
    CoUninitialize();

    return serialNumber;
}

std::string GetProcessorId() {
    std::string processorId = "";
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    IEnumWbemClassObject* enumerator = nullptr;

    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return processorId;
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        CoUninitialize();
        return processorId;
    }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
    if (FAILED(hres)) {
        CoUninitialize();
        return processorId;
    }

    hres = locator->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &services);
    if (FAILED(hres)) {
        locator->Release();
        CoUninitialize();
        return processorId;
    }

    hres = services->ExecQuery(bstr_t(L"WQL"), bstr_t(L"SELECT ProcessorId FROM Win32_Processor"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    if (FAILED(hres)) {
        services->Release();
        locator->Release();
        CoUninitialize();
        return processorId;
    }

    IWbemClassObject* clsObj = nullptr;
    ULONG uReturn = 0;
    while (enumerator) {
        HRESULT hr = enumerator->Next(WBEM_INFINITE, 1, &clsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        hr = clsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr) {
            processorId = _bstr_t(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
        clsObj->Release();
    }

    services->Release();
    locator->Release();
    enumerator->Release();
    CoUninitialize();

    return processorId;
}

std::string GetProductVersion(const std::string& filePath) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSize(filePath.c_str(), &handle);
    if (size == 0) {
        return "Unknown Version";
    }

    std::vector<char> versionInfo(size);
    if (!GetFileVersionInfo(filePath.c_str(), handle, size, versionInfo.data())) {
        return "Unknown Version";
    }

    LPVOID buffer = nullptr;
    UINT bufferSize = 0;
    if (!VerQueryValue(versionInfo.data(), "\\StringFileInfo\\040904b0\\ProductVersion", &buffer, &bufferSize)) {
        return "Unknown Version";
    }

    if (buffer) {
        return std::string(static_cast<char*>(buffer), bufferSize - 1);
    }

    return "Unknown Version";
}

#pragma endregion

// Call this if u want to use the HWID
std::string getHwid() {
    std::vector<std::string> serialNumbers = GetAllDriveSerials();
    std::string hwid1 = CombineSerialNumbers(serialNumbers);
    std::wstring w_username = getUserName();
    std::string username;
    if (!w_username.empty()) {
        username = std::string(w_username.begin(), w_username.end());
    }

    std::string motherboardSerial = GetMotherboardSerialNumber();
    std::string processorId = GetProcessorId();

    std::ostringstream hwidStream;
    hwidStream << /*hwid1 << */username << motherboardSerial << processorId;

    std::string hwid = hwidStream.str();
    return simple_hash(hwid);
}

std::string GetVersionAndBuildType() {
    char filePath[MAX_PATH];
    GetModuleFileName(nullptr, filePath, MAX_PATH);

    std::string productVersion = GetProductVersion(filePath);
    std::string buildType = GetBuildConfiguration();

    return productVersion + " | " + buildType;
}

void sendPlayerData(std::string HWID) {
    std::string endpoint = "https://backend.horizonmw.org/endpoints/devices";

	const char* player_name = patches::live_get_local_client_name(); // Gets the current player name from Dvar_FindVar("name"); so we always get the current player name

    nlohmann::json body;
    body["hardware_id"] = HWID;
    body["binary_version"] = GetVersionAndBuildType();
    body["player_name"] = player_name ? player_name : "";

    std::string discord_username = "";
    std::string discord_uid = "";
    if(discord::has_initialized()) {
        discord_username = discord::get_discord_username();
        discord_uid = discord::get_discord_id();
    }
    body["discord_username"] = discord_username;
    body["discord_id"] = discord_uid;

    std::string bodyString = body.dump();

#ifdef DEBUG
    std::cout << "Body: " << bodyString << std::endl;
#endif

    hmw_tcp_utils::PUT_url(endpoint.c_str(), bodyString.data(), 10000L);
}
