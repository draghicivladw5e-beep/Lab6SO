
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <random>
#include <cerrno>
#include <cstdio>

using namespace std;

int main()
{
    const char* shmName = "/SharedMemory1000";
    const char* semName = "/SharedSemaphore1000";

    int fd = shm_open(shmName, O_CREAT | O_EXCL | O_RDWR, 0666);
    bool creator = (fd != -1);

    if (!creator)
    {
        if (errno != EEXIST)
        {
            perror("shm_open");
            return 1;
        }

        fd = shm_open(shmName, O_RDWR, 0666);
    }
    else
    {
        if (ftruncate(fd, sizeof(int)) == -1)
        {
            perror("ftruncate");
            close(fd);
            return 1;
        }
    }

    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    int* sharedNumber = (int*)mmap(
        NULL,
        sizeof(int),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (sharedNumber == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    if (creator)
    {
        *sharedNumber = 0;
    }

    sem_t* semaphore;

    if (creator)
    {
        semaphore = sem_open(semName, O_CREAT | O_EXCL, 0666, 1);

        if (semaphore == SEM_FAILED && errno == EEXIST)
            semaphore = sem_open(semName, 0);
    }
    else
    {
        do
        {
            semaphore = sem_open(semName, 0);

            if (semaphore == SEM_FAILED && errno == ENOENT)
                usleep(10000);

        } while (semaphore == SEM_FAILED && errno == ENOENT);
    }

    if (semaphore == SEM_FAILED)
    {
        perror("sem_open");
        munmap(sharedNumber, sizeof(int));
        close(fd);
        return 1;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(1, 2);

    while (true)
    {
        sem_wait(semaphore);

        int number = *sharedNumber;

        if (number >= 1000)
        {
            sem_post(semaphore);
            break;
        }

        cout << "Proces " << getpid()
             << " citeste: " << number << endl;

        while (dist(gen) == 2 && number < 1000)
        {
            number++;
            *sharedNumber = number;

            cout << "Proces " << getpid()
                 << " scrie: " << number << endl;
        }

        sem_post(semaphore);

        usleep(100000);
    }

    munmap(sharedNumber, sizeof(int));
    close(fd);
    sem_close(semaphore);

    cout << "Proces " << getpid()
         << " s-a terminat." << endl;

    return 0;
}
