/*
 * daemonize.c
 * This example daemonizes a process, writes a few log messages,
 * sleeps 20 seconds and terminates afterwards.
 * This is an answer to the stackoverflow question:
 * https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux/17955149#17955149
 * Fork this code: https://github.com/pasce/daemon-skeleton-linux-c
 */
#include <atomic>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>

#include <ourcontract.h>

namespace {
    // In the GNUC Library, sig_atomic_t is a typedef for int,
    // which is atomic on all systems that are supported by the
    // GNUC Library
    volatile sig_atomic_t do_shutdown = 0;
  
    // std::atomic is safe, as long as it is lock-free
    std::atomic<bool> shutdown_requested = false;
    static_assert( std::atomic<bool>::is_always_lock_free );
    // or, at runtime: assert( shutdown_requested.is_lock_free() );

    FILE* fd_error;
    std::unordered_set<int> setContPID;
}

void shutdown_signal_handler(int signum)
{
  // ok, lock-free atomics
    do_shutdown = 1;
    shutdown_requested = true;
    std::cerr << "Shutdown contract daemon\n";
}


void appoint_log(std::string cont_dir)
{
    fd_error = fopen((cont_dir + "/err").c_str(), "w+");
}

extern "C" int contract_main(int argc, char **argv)
{
    if (!state_exist()) {
        signal(SIGTERM, shutdown_signal_handler);
        std::string cont_dir = contract_daemon();
        
        if(cont_dir == "") {
            //std::cout << "Daemon failed\n";
            return EXIT_SUCCESS;
        }
        appoint_log(cont_dir);

        while (!do_shutdown && !shutdown_requested.load())
        {
            fprintf(fd_error, "%d contract daemon\n", getpid());
            fflush(fd_error);
            sleep(5);
        }
        fprintf(fd_error, "contract daemon exit\n");
        fflush(fd_error);
        
        fclose(fd_error);
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
