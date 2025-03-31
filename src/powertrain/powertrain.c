#include "powertrain_func.h"

int main()
{
    if (!init_logging_system())
    {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    sock_receiver = create_can_socket(CAN_INTERFACE);
    sock_sender = create_can_socket(CAN_INTERFACE);
    if (sock_receiver < 0 || sock_sender < 0)
    {
        return ERROR_CODE;
    }

    pthread_mutex_init(&mutex_powertrain, NULL);

    pthread_t thread_start_stop;
    pthread_t thread_comms;

    pthread_create(&thread_start_stop, NULL, function_start_stop, &rec_data);
    pthread_create(&thread_comms, NULL, comms, NULL);

    pthread_join(thread_start_stop, NULL);
    pthread_join(thread_comms, NULL);

    pthread_mutex_destroy(&mutex_powertrain);

    close_can_socket(sock_receiver);
    close_can_socket(sock_sender);

    cleanup_logging_system();

    return 0;
}