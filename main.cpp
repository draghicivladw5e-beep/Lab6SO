
#include <iostream>
#include <windows.h>
#include <random>

using namespace std;

int main()
{
    HANDLE hMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(int),
        "SharedMemory1000"
    );

    if (hMap == NULL)
    {
        cout << "Eroare la crearea memoriei!" << endl;
        return 1;
    }

    int* sharedNumber = (int*)MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(int)
    );

    if (sharedNumber == NULL)
    {
        cout << "Eroare la maparea memoriei!" << endl;
        CloseHandle(hMap);
        return 1;
    }

    HANDLE hSemaphore = CreateSemaphoreA(
        NULL,
        1,
        1,
        "SharedSemaphore1000"
    );

    if (hSemaphore == NULL)
    {
        cout << "Eroare la crearea semaforului!" << endl;
        UnmapViewOfFile(sharedNumber);
        CloseHandle(hMap);
        return 1;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(1, 2);

    while (true)
    {
        WaitForSingleObject(hSemaphore, INFINITE);

        int number = *sharedNumber;

        if (number >= 1000)
        {
            ReleaseSemaphore(hSemaphore, 1, NULL);
            break;
        }

        cout << "Proces " << GetCurrentProcessId()
             << " citeste: " << number << endl;

        while (dist(gen) == 2 && number < 1000)
        {
            number++;
            *sharedNumber = number;

            cout << "Proces " << GetCurrentProcessId()
                 << " scrie: " << number << endl;
        }

        ReleaseSemaphore(hSemaphore, 1, NULL);
    }

    UnmapViewOfFile(sharedNumber);
    CloseHandle(hSemaphore);
    CloseHandle(hMap);

    cout << "Proces " << GetCurrentProcessId()
         << " s-a terminat." << endl;

    return 0;
}
