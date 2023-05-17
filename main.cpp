#include <windows.h>
#include <iostream>
#include <string>

unsigned char shell[] = {
    0x6A, 0x00,                   // push   0x0             -> UINT    uType
    0x68, 0xFF, 0xFF, 0xFF, 0x00, // push   0xffffff        -> LPCTSTR lpCaption
    0x68, 0xFF, 0xFF, 0xFF, 0x00, // push   0xffffff        -> LPCTSTR lpText
    0x6A, 0x00,                   // push   0x0             -> HWND    hWnd
    0xB8, 0xFF, 0xFF, 0xFF, 0x00, // mov    eax,0xffffff    -> MessageBoxA PTR
    0xFF, 0xD0,                   // call   eax
    0x31, 0xC0,                   // xor    eax,eax
    0xC3                          // ret
};

int main()
{

    auto getProcessId = []()
    {
        int processId;
        std::cout << "Enter process id: ";
        std::cin >> processId;
        if (!processId)
        {
            std::cout << "Error: process id is null" << std::endl;
        }

        return processId;
    };

    auto hProcess = [=, &getProcessId]()
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, getProcessId());
        if (!hProcess)
        {
            std::cout << "Error: OpenProcess" << std::endl;
        }
        return hProcess;
    }();

    std::cout << "Process Handle: 0x" << std::hex << hProcess << std::endl;

    HMODULE hUser32 = LoadLibraryA("User32.dll");

    auto hMessageBoxA = GetProcAddress(hUser32, "MessageBoxA");

    if (!hMessageBoxA)
    {
        std::cout << "Error: GetProcAddress" << std::endl;
    }

    std::cout << "Handle MessageBoxA: 0x" << std::hex << hMessageBoxA << std::endl;

    std::string lpText, lpCaption; // lpText, lpCaption
    [&]()
    {
        std::cout << "Enter lpText: ";
        std::cin >> lpText;
        std::getline(std::cin, lpText, '\n');
        std::cout << "Enter lpCaption: ";
        std::getline(std::cin, lpCaption, '\n');
    }();

    DWORD_PTR allocMemory = reinterpret_cast<DWORD_PTR>(VirtualAllocEx(hProcess, NULL, sizeof(shell) + sizeof(shell) + sizeof(lpText) + sizeof(lpCaption), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)); // alloc memory

    if (!allocMemory) {
        std::cout << "Error: VirtualAllocEx" << std::endl;
        return 0;
    }

    std::cout << "Alloc Memory: 0x" << std::hex << allocMemory << std::endl;

    *(DWORD_PTR *)(&shell[0] + 15) = reinterpret_cast<DWORD_PTR>(hMessageBoxA); // mov eax, 0xffffff -> mov eax, hMessageBoxA

    DWORD lpTExtOffset = static_cast<DWORD>(sizeof(shell));

    *(DWORD *)(&shell[0] + 3) = static_cast<DWORD>(allocMemory + lpTExtOffset); // push   0xffffff -> to new ptr
    DWORD lpCaptionOffset = static_cast<DWORD>(sizeof(shell) + sizeof(lpCaption));
    *(DWORD *)(&shell[0] + 8) = static_cast<DWORD>(allocMemory + lpCaptionOffset); // push   0xffffff -> to new ptr

    WriteProcessMemory(hProcess, reinterpret_cast<void *>(allocMemory), shell, sizeof(shell), NULL); // write shell

    WriteProcessMemory(hProcess, reinterpret_cast<void *>(allocMemory + lpTExtOffset), static_cast<void *>(&lpText), sizeof(lpText), NULL); // write lpText

    WriteProcessMemory(hProcess, reinterpret_cast<void *>(allocMemory + lpCaptionOffset), static_cast<void *>(&lpCaption), sizeof(lpCaption), NULL); // write lpCaption

    CreateRemoteThreadEx(hProcess, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(allocMemory), 0, 0, 0, NULL); // create thread

    VirtualFreeEx(hProcess, reinterpret_cast<void *>(shell), sizeof(shell), MEM_RELEASE); // free memory

    std::cin.get();
    return 0;
}