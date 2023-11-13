#include <iostream>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include <sys/mman.h>

#include "shahed_memory.hpp"

const float EPS = 1e-10;

int main(int argc, char* argv[]) {
    int fd = open(argv[1], O_CREAT | O_WRONLY, S_IRWXU);
    if (fd == -1) {
        std::cerr << "Ошибка при открытии файла для записи ответов" << '\n';
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, 0) == -1) {
        std::cerr << "Ошибка при очистке файла" << '\n';
        exit(EXIT_FAILURE);
    }

    int shared_memory_fd = shm_open(shared_memory_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shared_memory_fd == -1) {
        std::cerr << "Ошибка при открытии shared memory" << '\n';
        exit(EXIT_FAILURE);
    }

    SharedMemory* shared_memory = (SharedMemory*)mmap(NULL, sizeof(SharedMemory), prots, flags, shared_memory_fd, 0);
    if (shared_memory == MAP_FAILED) {
        std::cerr << "Ошибка при отображении shared_memory" << '\n';
        exit(EXIT_FAILURE);
    }

    std::string s, temp;
    int first_gap_index;
    float first_number;
    std::vector<float> numbers;

    while(1) {
        s = "";
        while(1) {
            sem_wait(&shared_memory->semaphore1);;
            s += std::string(shared_memory->data);
            sem_post(&shared_memory->semaphore2);
            if (std::string(shared_memory->data).size() < DataSize || shared_memory->flag == 0) {
                shared_memory->flag = 1;
                break;
            }
        }
        s += '\n';

        for (int i = 0; i < s.size(); ++i) {
            if (s[i] == ' ' || s[i] == '\n') {
                first_gap_index = i;
                break;
            }
        }

        temp = "";
        first_number = std::stof(s.substr(0, first_gap_index + 1));
        for (int i = first_gap_index + 1; i < s.size(); ++i) {
            if (s[i] == ' ' || s[i] == '\n') {
                numbers.push_back(std::stof(temp));
                temp = "";
                continue;
            }
            temp += s[i];
        }

        for (int i = 0; i < numbers.size(); ++i) {
            if (numbers[i] < EPS && numbers[i] > -EPS) {
                std::cerr << "Ошибка при попытке деления на 0" << '\n';
                shared_memory->flag = 0;
                sem_post(&shared_memory->semaphore3);
                exit(EXIT_FAILURE);
            }
            first_number /= numbers[i];
        }
        numbers.clear();
        
        shared_memory->value = first_number;
        dprintf(fd, "%f\n", first_number);
        sem_post(&shared_memory->semaphore3);
    }

    munmap(shared_memory, sizeof(SharedMemory));
    close(fd);

    return 0;
}