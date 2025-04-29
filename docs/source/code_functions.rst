Code Functions
===============

Check Disable Engine
-----------------------------------
.. _check_disable_engine:

.. c:function:: void check_disable_engine(VehicleData *ptr_rec_data)

   Implements requirement :ref:`SWR5.1`

   This function evaluates engine disable conditions based on sensor readings
   such as speed, acceleration, brake status, and temperature.

   It logs system messages and CAN errors according to defined failure modes.

   File: ``powertrain/powertrain_func.c``

.. literalinclude:: ../../src/powertrain/powertrain_func.c
   :language: c
   :lines: 107-182
   :caption: check_disable_engine function implementation

Handle Engine Restart Logic
-----------------------------------
.. _handle_engine_restart_logic: 

.. c:function:: void handle_engine_restart_logic(VehicleData *data)

   Implements requirement :ref:`SWR1.2`

   This function handles the logic for restarting the engine based on 
   current vehicle conditions.

   It ensures that the engine is restarted only when it is safe to do so.

   File: ``powertrain/powertrain_func.c``

.. literalinclude:: ../../src/powertrain/powertrain_func.c
   :language: c
   :lines: 188-219
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
   :lines: 225-263
   :caption: function_start_stop function implementation

Print Dashboard status
-----------------------------------
.. _print_dashboard_status:

.. c:function:: void print_dashboard_status()

   Implements requirement :ref:`SWR5.2` and :ref:`SWR6.3`

   This function prints the current status of the dashboard, including
   system warnings, start/stop and battery status.

   It displays real-time relevant status information to the driver.

   File: ``dashboard/dashboard_func.c``

.. literalinclude:: ../../src/dashboard/dashboard_func.c
   :language: c
   :lines: 39-50
   :caption: print_dashboard_status function implementation

Parse Input Received
-----------------------------------
.. _parse_input_received:

.. c:function:: void parse_input_received(char *input)

   Implements requirement :ref:`SWR1.3`, :ref:`SWR4.5`, :ref:`SWR5.1` and :ref:`SWR5.3`

   This function parses the data received from the CAN and updates the
   system state accordingly.

   It processes user commands, engine commands, sensor readings and errors
   to ensure the system responds correctly.

   File: ``dashboard/dashboard_func.c``

.. literalinclude:: ../../src/dashboard/dashboard_func.c
   :language: c
   :lines: 59-65
   :caption: parse_input_received function implementation
