#include <windows.h>
#include <iostream>
#include <ctime>

// Configurare
const char* SHM_NAME = "Local\\SharedMemFinal";
const char* SEM_NAME = "Local\\SemFinal";
const int TARGET = 1000;

int main() {
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    // 1. Memorie Partajata (Memory Mapped File)
    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), SHM_NAME);
    if (!hMapFile) { std::cerr << "Err CreateFileMapping\n"; return 1; }

    int* pBuf = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (!pBuf) { std::cerr << "Err MapViewOfFile\n"; CloseHandle(hMapFile); return 1; }

    // 2. Semafor (Initial 1, Max 1)
    HANDLE hSemaphore = CreateSemaphoreA(NULL, 1, 1, SEM_NAME);
    if (!hSemaphore) { std::cerr << "Err CreateSemaphore\n"; UnmapViewOfFile(pBuf); CloseHandle(hMapFile); return 1; }

    printf("Windows Process [PID %lu] started.\n", GetCurrentProcessId());

    while (true) {
        // Asteapta semaforul
        DWORD dwWaitResult = WaitForSingleObject(hSemaphore, INFINITE);

        if (dwWaitResult == WAIT_OBJECT_0) {
            // -- ZONA CRITICA --
            
            // "citesc memoria"
            if (*pBuf >= TARGET) {
                ReleaseSemaphore(hSemaphore, 1, NULL);
                break;
            }

            // "dau cu banul cat timp cade 2"
            int banul = (rand() % 2) + 1; // 1 sau 2

            while (banul == 2 && *pBuf < TARGET) {
                (*pBuf)++;
                std::cout << "[PID " << GetCurrentProcessId() << "] a dat 2 -> Scriu: " << *pBuf << std::endl;

                if (*pBuf >= TARGET) break;

                // Dau din nou
                banul = (rand() % 2) + 1;
            }

            // "eliberează semaforul"
            ReleaseSemaphore(hSemaphore, 1, NULL);
            // -- SFARSIT ZONA CRITICA --
        } 
        else { break; }

        Sleep(10); // 10ms pauza
    }

    std::cout << "Finalizat. Numar: " << *pBuf << std::endl;

    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);
    return 0;
}