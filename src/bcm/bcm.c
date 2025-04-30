#include "bcm_func.h"

#define CAN_INTERFACE       ("vcan0")
#define ERROR_CODE          (1)

int main(void)
{
    // Create CAN send socket using the defined interface (vcan0)
    sock_send = create_can_socket(CAN_INTERFACE);
    if (sock_send < 0)
    {
        return EXIT_FAILURE;
    }

    // Create CAN recv socket using the defined interface (vcan0)
    sock_recv = create_can_socket(CAN_INTERFACE);
    if (sock_recv < 0)
    {
        return ERROR_CODE;
    }

    if (!init_logging_system()) {
        fprintf(stderr, "Failed to open log file for writing.\n");
        return ERROR_CODE;
    }

    // Set simulation order to RUN
    simu_order = ORDER_RUN;

    // Initialize semaphores
    //sem_init(&sem_stop_start, 0, 1);    // Stop/Start can produce initially
    sem_init(&sem_comms, 0, 0);         // Comms must wait

    // Initialize mutex for simulation
    pthread_mutex_init(&mutex_bcm, NULL);

    pthread_t thread_speed;
    pthread_t thread_comms;
    pthread_t thread_battery;

    // Start simulation threads: speed simulation, communication, and battery sensor
    pthread_create(&thread_speed, NULL, simu_speed, vehicle_data);
    pthread_create(&thread_comms, NULL, comms, NULL);
    pthread_create(&thread_battery, NULL, sensor_battery, NULL);

    // Wait for threads to finish (in a real simulation these might run indefinitely)
    pthread_join(thread_speed, NULL);
    pthread_join(thread_comms, NULL);
    pthread_join(thread_battery, NULL);

    // Clean up
    pthread_mutex_destroy(&mutex_bcm);

    //sem_destroy(&sem_stop_start);
    sem_destroy(&sem_comms);

    close_can_socket(sock_send);
    close_can_socket(sock_recv);
    cleanup_logging_system();

    return EXIT_SUCCESS;
}
