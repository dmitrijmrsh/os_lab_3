#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <vector>

#include "shahed_memory.hpp"

int Min(int a, int b) {
    return (a <= b) ? a : b;
}

int main() {
    std::cout.precision(6);
    std::cout << std::fixed;
    int temp;
    std::vector<float> results;
    std::string name, str;
    std::cout << "Enter the name of file: ";
    std::getline(std::cin, name);

    //Открываю область shared memory
    int shared_memory_fd = shm_open(shared_memory_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shared_memory_fd == -1) {
        std::cerr << "Ошибка при открытии shared memory" << '\n';
        exit(EXIT_FAILURE);
    }

    //Изменяю размер shared_memory до размера структуры
    if (ftruncate(shared_memory_fd, sizeof(SharedMemory)) == -1) {
        std::cerr << "Ошибка при изменении размкера shared memory" << '\n';
        exit(EXIT_FAILURE);
    }

    //Отображаю участок памяти
    SharedMemory* shared_memory = (SharedMemory*)mmap(NULL, sizeof(SharedMemory), prots, flags, shared_memory_fd, 0);
    if (shared_memory == MAP_FAILED) {
        std::cerr << "Ошибка при отображении shared_memory" << '\n';
        exit(EXIT_FAILURE);
    }

    //Инициализация семафоров
    if (sem_init(&shared_memory->semaphore1, 1, 0) == -1) {
        std::cerr << "Ошибка при инициализации семафора 1" << '\n';
        exit(EXIT_FAILURE);
    }

    if (sem_init(&shared_memory->semaphore2, 1, 0) == -1) {
        std::cerr << "Ошибка при инициализации семафора 2" << '\n';
        exit(EXIT_FAILURE);
    }

    if (sem_init(&shared_memory->semaphore3, 1, 0) == -1) {
        std::cerr << "Ошибка при инициализации семафора 3" << '\n';
        exit(EXIT_FAILURE);
    }

    //Создаём дочерний процесс
    pid_t pid = fork();
    if (pid == -1) {
        std::cout << "Ошибка при создании дочернего процесса" << '\n';
        exit(EXIT_FAILURE);
    }

    //Дочерний процесс
    if (pid == 0) {
        temp = execl("child", "child", name.c_str(), NULL);
        if (temp == -1) {
            std::cerr << "Ошибка при запуске программы в дочернем процессе" << '\n';
            exit(EXIT_FAILURE);
        }
    }

    shared_memory->flag = 1;

    //Родительский процесс
    if (pid > 0) {
        bool first_number_flag = true;
        int start, finish;
        while (std::getline(std::cin, str)) {
            if (!shared_memory->flag) {
                exit(EXIT_FAILURE);
            }
            if (!first_number_flag) {
                results.push_back(shared_memory->value);
            }
            start = 0;
            finish = str.size();
            while(1) {
                if (DataSize == finish - start) {
                    shared_memory->flag = 0;
                }
                strncpy(shared_memory->data, str.substr(start, Min(DataSize, finish - start)).c_str(), sizeof(shared_memory->data));
                start += Min(DataSize, finish - start);
                sem_post(&shared_memory->semaphore1);
                sem_wait(&shared_memory->semaphore2);
                if (start == finish) {
                    break;
                }
            }
            first_number_flag = false;
            sem_wait(&shared_memory->semaphore3);
        }

        if (shared_memory->flag) {
            results.push_back(shared_memory->value);
        }

        for (float elem : results) {
            std::cout << elem << '\n';
        }

        sem_destroy(&shared_memory->semaphore1);
        sem_destroy(&shared_memory->semaphore2);
        sem_destroy(&shared_memory->semaphore3);

        munmap(shared_memory, sizeof(SharedMemory));
        shm_unlink(shared_memory_name);
    }

    return 0;
}