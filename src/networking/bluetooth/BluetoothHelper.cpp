#include "BluetoothHelper.h"
#include <sstream>
#include <iomanip>

namespace AnshuBio {

bool BluetoothHelper::IsBluetoothRadioAvailable() {
    BLUETOOTH_FIND_RADIO_PARAMS params{};
    params.dwSize = sizeof(params);
    HANDLE hRadio = nullptr;
    HBLUETOOTH_RADIO_FIND hFind = BluetoothFindFirstRadio(&params, &hRadio);
    if (hFind) {
        CloseHandle(hRadio);
        BluetoothFindRadioClose(hFind);
        return true;
    }
    return false;
}

std::vector<BluetoothDeviceInfo> BluetoothHelper::ScanPairedDevices() {
    std::vector<BluetoothDeviceInfo> devices;
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams{};
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = TRUE;
    searchParams.fReturnRemembered = TRUE;
    searchParams.fReturnUnknown = FALSE;
    searchParams.fReturnConnected = TRUE;
    searchParams.fIssueInquiry = FALSE;
    searchParams.cTimeoutMultiplier = 2;

    BLUETOOTH_DEVICE_INFO deviceInfo{};
    deviceInfo.dwSize = sizeof(deviceInfo);

    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (hFind) {
        do {
            BluetoothDeviceInfo info;
            char nameA[256] = { 0 };
            WideCharToMultiByte(CP_UTF8, 0, deviceInfo.szName, -1, nameA, sizeof(nameA), nullptr, nullptr);
            info.name = nameA;
            info.isConnected = deviceInfo.fConnected;
            info.isPaired = deviceInfo.fRemembered || deviceInfo.fAuthenticated;

            std::stringstream ss;
            for (int i = 5; i >= 0; --i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(deviceInfo.Address.rgBytes[i]);
                if (i > 0) ss << ":";
            }
            info.address = ss.str();
            devices.push_back(info);
        } while (BluetoothFindNextDevice(hFind, &deviceInfo));
        BluetoothFindDeviceClose(hFind);
    }
    return devices;
}

bool BluetoothHelper::RegisterSDPService(SOCKET rfcommSocket, const std::string& serviceName, const GUID& serviceGuid) {
    (void)rfcommSocket;
    WSAQUERYSETW qs{};
    ZeroMemory(&qs, sizeof(qs));
    qs.dwSize = sizeof(qs);
    
    std::wstring wServiceName(serviceName.begin(), serviceName.end());
    qs.lpszServiceInstanceName = const_cast<LPWSTR>(wServiceName.c_str());
    
    GUID guid = serviceGuid;
    qs.lpServiceClassId = &guid;
    qs.dwNameSpace = NS_BTH;
    qs.dwNumberOfCsAddrs = 0;

    if (WSASetServiceW(&qs, RNRSERVICE_REGISTER, 0) == 0) {
        return true;
    }
    return false;
}

} // namespace AnshuBio
