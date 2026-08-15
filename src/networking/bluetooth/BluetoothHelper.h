#pragma once
#include <winsock2.h>
#include <windows.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>
#include <ws2bth.h>
#include <string>
#include <vector>

#pragma comment(lib, "bthprops.lib")
#pragma comment(lib, "ws2_32.lib")

namespace AnshuBio {

struct BluetoothDeviceInfo {
    std::string name;
    std::string address;
    bool isConnected = false;
    bool isPaired = false;
};

class BluetoothHelper {
public:
    static bool IsBluetoothRadioAvailable();
    static std::vector<BluetoothDeviceInfo> ScanPairedDevices();
    static bool RegisterSDPService(SOCKET rfcommSocket, const std::string& serviceName, const GUID& serviceGuid);
};

} // namespace AnshuBio
