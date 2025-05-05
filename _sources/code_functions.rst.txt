Code Functions
===============

Check Disable Engine
-----------------------------------
.. _check_disable_engine:

.. c:function:: void check_disable_engine(VehicleData *ptr_rec_data)

   Implements requirement :ref:`SWR2.2`, :ref:`SWR2.3`, :ref:`SWR2.4`,
   :ref:`SWR2.6`, :ref:`SWR2.7`, :ref:`SWR2.8`, :ref:`SWR2.9`,
   :ref:`SWR4.3`, :ref:`SWR4.4`, and :ref:`SWR5.1`.

   This function evaluates engine disable conditions based on sensor readings
   such as speed, acceleration, brake status, and temperature.

   It logs system messages and CAN errors according to defined failure modes.

   File: ``powertrain/powertrain_func.c``

.. literalinclude:: ../../src/powertrain/powertrain_func.c
   :language: c
   :lines: 118-195
   :caption: check_disable_engine function implementation

Handle Engine Restart Logic
-----------------------------------
.. _handle_engine_restart_logic: 

.. c:function:: void handle_engine_restart_logic(VehicleData *data)

   Implements requirement :ref:`SWR1.1`, :ref:`SWR2.5`, :ref:`SWR3.1`, :ref:`SWR3.4`, and :ref:`SWR3.5`.

   This function handles the logic for restarting the engine based on 
   current vehicle conditions.

   It ensures that the engine is restarted only when it is safe to do so.

   File: ``powertrain/powertrain_func.c``

.. literalinclude:: ../../src/powertrain/powertrain_func.c
   :language: c
   :lines: 205-252
   :caption: handle_engine_restart_logic function implementation

Function Start Stop
-----------------------------------
.. _*function_start_stop:

.. c:function:: void *function_start_stop(void *arg)

   Implements requirement :ref:`SWR1.2`

   This function is responsible for check if start/stop was
   activated by the driver.

   It manages the start/stop state and ensures that it operates properly.

   File: ``powertrain/powertrain_func.c``

.. literalinclude:: ../../src/powertrain/powertrain_func.c
   :language: c
   :lines: 258-296
   :caption: function_start_stop function implementation

Parse Input Received
-----------------------------------
.. _parse_input_received:

.. c:function:: void parse_input_received(char *input)

   Implements requirement :ref:`SWR1.2`, :ref:`SWR1.3`, :ref:`SWR1.4`,
   :ref:`SWR4.5`, :ref:`SWR5.1`, :ref:`SWR5.2`, :ref:`SWR5.3`, and :ref:`SWR6.3`

   This function parses the data received from the CAN and updates the
   system state accordingly.

   It processes user commands, engine commands, sensor readings and errors
   to ensure the system responds correctly.

   File: ``dashboard/dashboard_func.c``

.. literalinclude:: ../../src/dashboard/dashboard_func.c
   :language: c
   :lines: 46-52
   :caption: parse_input_received function implementation

Parse Input Received Powertrain
-----------------------------------
.. _parse_input_received_powertrain:

.. c:function:: void parse_input_received_powertrain(char *input)

   Implements requirement :ref:`SWR1.2`.

   This function parses the data received from the CAN and updates the
   system state accordingly on the Powertrain module.

   It processes user commands, engine commands, sensor readings and errors
   to ensure the system responds correctly.

   File: ``powertrain/can_comms.c``

.. literalinclude:: ../../src/powertrain/can_comms.c
   :language: c
   :lines: 26-119
   :caption: parse_input_received_powertrain function implementation

Send Encrypted Message
-----------------------------------
.. _send_encrypted_message:

.. c:function:: void send_encrypted_message(int sock, const char *message, int can_id)

   Implements requirement :ref:`SWR1.4`.

   This function sends an encrypted message via CAN socket.

   File: ``common_includes/can_socket.c``

.. literalinclude:: ../../src/common_includes/can_socket.c
   :language: c
   :lines: 167-193
   :caption: send_encrypted_message function implementation

Log Toggle Event
-----------------------------------
.. _log_toggle_event:

.. c:function:: void log_toggle_event(char* message)

   Implements requirement :ref:`SWR1.5`.

   This function logs events in a file.

   File: ``common_includes/logging.c``

.. literalinclude:: ../../src/common_includes/logging.c
   :language: c
   :lines: 31-53
   :caption: log_toggle_event function implementation

Check User Input Command
-----------------------------------
.. _check_input_command:

.. c:function:: void check_input_command(char* option, int socket)

   Implements requirement :ref:`SWR1.5`.

   This function checks inputs received by the instrument cluster.

   File: ``instrument_cluster/instrument_cluster_func.c``

.. literalinclude:: ../../src/instrument_cluster/instrument_cluster_func.c
   :language: c
   :lines: 17-29
   :caption: check_input_command function implementation

Read CSV
-----------------------------------
.. _read_csv:

.. c:function:: void read_csv(const char *path)

   Implements requirement :ref:`SWR2.1` and :ref:`SWR4.2`.

   This function reads the simulation data from the CSV file.

   File: ``bcm/bcm_func.c``

.. literalinclude:: ../../src/bcm/bcm_func.c
   :language: c
   :lines: 92-154
   :caption: read_csv function implementation

Check Health Signals
-----------------------------------
.. _check_health_signals:

.. c:function:: void check_health_signals(void)

   Implements requirement :ref:`SWR6.1`, :ref:`SWR6.2`, :ref:`SWR6.3`, and :ref:`SWR6.4`.

   This function evaluates system health.

   File: ``bcm/bcm_func.c``

.. literalinclude:: ../../src/bcm/bcm_func.c
   :language: c
   :lines: 409-458
   :caption: check_health_signals function implementation
