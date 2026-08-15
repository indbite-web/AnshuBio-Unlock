#include "WindowsService.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--install") {
            return AnshuBio::WindowsService::Instance().InstallService() ? 0 : 1;
        }
        if (arg == "--uninstall") {
            return AnshuBio::WindowsService::Instance().UninstallService() ? 0 : 1;
        }
        if (arg == "--service") {
            return AnshuBio::WindowsService::Instance().RunService() ? 0 : 1;
        }
    }
    // Default when launched by SCM or without arguments
    return AnshuBio::WindowsService::Instance().RunService() ? 0 : 1;
}
