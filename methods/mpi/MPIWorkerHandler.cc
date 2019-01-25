#include <string>
#include <thread>

#include "types.h"

#include "mpi_utils.h"
#include "mpi_common.h"
#include "spawn.h"
#include "MPIWorkerHandler.h"
#include "AbstractWorkerHandler.h"

#ifndef NDEBUG
#include <iostream>
#endif

MPIWorkerHandler::MPIWorkerHandler(
        const cmd_t& command,
        const std::string& input_string) :
    AbstractWorkerHandler(command, input_string)
{
#ifndef NDEBUG
    const int rank = MPI::COMM_WORLD.Get_rank();
    const int size = MPI::COMM_WORLD.Get_size();
    std::cerr << "Manager " << rank << "/" << size << ": MPI process constructing...\n";
#endif

    // Spawn MPI child process
    m_child_comm = spawn_worker(m_command);

    // Write input string to spawned MPI process
    m_child_comm.Send(input_string.c_str(), input_string.size() + 1, MPI::CHAR,
            WORKER_RANK, MANAGER_MSG_TAG);
}

MPIWorkerHandler::~MPIWorkerHandler()
{
#ifndef NDEBUG
    const int rank = MPI::COMM_WORLD.Get_rank();
    const int size = MPI::COMM_WORLD.Get_size();
    std::cerr << "Manager " << rank << "/" << size << ": MPI simulation destroying...\n";
#endif
    terminate();

    // Free communicator
#ifndef NDEBUG
    std::cerr << "Manager " << rank << "/" << size << ": disconnecting child communicator...\n";
#endif
    m_child_comm.Disconnect();
#ifndef NDEBUG
    std::cerr << "Manager " << rank << "/" << size << ": child communicator disconnected!\n";
#endif
}

void MPIWorkerHandler::terminate()
{
#ifndef NDEBUG
    const int rank = MPI::COMM_WORLD.Get_rank();
    const int size = MPI::COMM_WORLD.Get_size();
    std::cerr << "Manager " << rank << "/" << size << ": MPI simulation terminating...\n";
#endif
    // MPI does not provide process control, so
    // we can only wait for the simulation to finish
    // if it has not finished yet
    if (!m_result_received)
    {
        // Timeout if message is not ready yet
        while (!m_child_comm.Iprobe(WORKER_RANK, WORKER_MSG_TAG))
            std::this_thread::sleep_for(MAIN_TIMEOUT);

        // Receive message
        receiveMessage();

        // Receive error code
        receiveErrorCode();

        // Set flag
        m_result_received = true;
    }
#ifndef NDEBUG
    std::cerr << "Manager " << rank << "/" << size << ": MPI simulation terminated!\n";
#endif
}

bool MPIWorkerHandler::isDone()
{
    // Probe for result if result has not yet been received
    if (    !m_result_received &&
            m_child_comm.Iprobe(WORKER_RANK, WORKER_MSG_TAG) )
    {
        // Receive message
        m_output_buffer.assign(receiveMessage());

        // Receive error code
        m_error_code = receiveErrorCode();

        // Set flag
        m_result_received = true;
    }

    return m_result_received;
}

std::string MPIWorkerHandler::receiveMessage() const
{
    return receive_string(m_child_comm, WORKER_RANK, WORKER_MSG_TAG);
}

int MPIWorkerHandler::receiveErrorCode() const
{
    return receive_integer(m_child_comm, WORKER_RANK, WORKER_ERROR_CODE_TAG);
}
