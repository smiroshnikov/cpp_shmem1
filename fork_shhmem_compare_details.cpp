#include <iostream>
#include <iomanip> // Include for setting output precision
#include <unistd.h> // Include for fork()
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <string>
#include <cstdlib> // Include for generating random string

// Function to generate a random string of a given length
std::string generateRandomString(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result += charset[rand() % charset.length()];
    }
    return result;
}

int main() {
    // Get the parent process ID (PID) and print it
    pid_t parentPid = getpid();
    std::cout << "Parent (PID " << parentPid << "): Process started." << std::endl;

    struct timeval start, end; // For measuring time

    // Create a pipe for IPC
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        std::cerr << "Pipe creation failed." << std::endl;
        return 1;
    }

    // Shared memory key
    key_t shmKey = ftok("/tmp", 'X');
    int shmid = shmget(shmKey, 1024 * 1024, IPC_CREAT | 0666); // 1MB shared memory

    // Create a child process
    pid_t childPid = fork();

    if (childPid < 0) {
        std::cerr << "Fork failed." << std::endl;
        return 1; // Handle the error
    }

    if (childPid == 0) {
        // This is the child process (with shared memory)

        // Get the child's process ID (PID) and print it
        pid_t childProcessId = getpid();
        std::cout << "Child (PID " << childProcessId << "): Process started (with shared memory)." << std::endl;

        // Attach the shared memory segment to the child process
        char* sharedMemory = static_cast<char*>(shmat(shmid, nullptr, 0));

        gettimeofday(&start, nullptr); // Start measuring time

        // Read the random string from shared memory
        std::string randomString(sharedMemory);

        gettimeofday(&end, nullptr); // Stop measuring time
        double elapsedTimeSharedMem = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6; // Convert to seconds

        std::cout << "Child (PID " << childProcessId << "): Received Value (shared memory) = " << randomString.length() << " bytes" << std::endl;
        std::cout << "Child (PID " << childProcessId << "): Time taken for IPC (shared memory) = " << std::fixed << std::setprecision(6) << elapsedTimeSharedMem << " seconds" << std::endl;
    } else {
        // This is the parent process (with shared memory)

        // Get the parent's process ID (PID) and print it
        pid_t parentProcessId = getpid();
        std::cout << "Parent (PID " << parentProcessId << "): Process started (with shared memory)." << std::endl;

        // Attach the shared memory segment to the parent process
        char* sharedMemory = static_cast<char*>(shmat(shmid, nullptr, 0));

        std::string dataToSend = generateRandomString(1024); // A smaller random string for pipe-based communication

        gettimeofday(&start, nullptr); // Start measuring time

        // Write the random string to shared memory
        dataToSend.copy(sharedMemory, dataToSend.size());

        gettimeofday(&end, nullptr); // Stop measuring time
        double elapsedTimeSharedMem = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6; // Convert to seconds

        // Optional: Remove the shared memory segment
        shmctl(shmid, IPC_RMID, nullptr);

        // Close the read end of the pipe (parent will only write)
        close(pipefd[0]);

        gettimeofday(&start, nullptr); // Start measuring time

        // Write the random string to the child through the pipe
        write(pipefd[1], dataToSend.c_str(), dataToSend.size());

        gettimeofday(&end, nullptr); // Stop measuring time
        double elapsedTimePipe = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6; // Convert to seconds

        std::cout << "Parent (PID " << parentProcessId << "): Sent Value (pipe) = " << dataToSend.length() << " bytes" << std::endl;
        std::cout << "Parent (PID " << parentProcessId << "): Time taken for IPC (pipe) = " << std::fixed << std::setprecision(6) << elapsedTimePipe << " seconds" << std::endl;

        // Optional: Close the write end of the pipe
        close(pipefd[1]);

        // Calculate and print the time difference
        double timeDifference = elapsedTimeSharedMem - elapsedTimePipe;
        std::cout << "Time Difference (Shared Memory - Pipe) = " << std::fixed << std::setprecision(6) << timeDifference << " seconds" << std::endl;

        // Print whether shared memory was faster or non-shared memory was faster
        if (timeDifference < 0) {
            std::cout << "SHARED WAS FASTER" << std::endl;
        } else if (timeDifference > 0) {
            std::cout << "NON-SHARED WAS FASTER" << std::endl;
        } else {
            std::cout << "SHARED AND NON-SHARED TOOK THE SAME TIME" << std::endl;
        }
    }

    return 0; // Both parent and child processes exit here
}
