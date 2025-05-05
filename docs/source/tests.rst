Test Cases
==========

Check Button press_start_stop Command
---------------------------------------
.. _test_press_start_stop:

.. c:function:: static void test_press_start_stop(void)

   Implements tests for :ref:`SWR1.1` and :ref:`SWR1.5`.
   
   This function tests if internal state updates when 'press_start_stop' is received.

   File: ``unit/test_instrument_cluster.c``

.. literalinclude:: ../../tests/unit/test_instrument_cluster.c
   :language: c
   :lines: 35-55
   :caption: tests/unit/test_instrument_cluster.c (test_press_start_stop)


Test Logging Under Concurrency
---------------------------------------
.. _test_logging_concurrency:

.. c:function:: static void test_logging_concurrency(void)

   Implements tests for :ref:`SWR1.5`.
   
   This function tests if logging works under under concurrent access.

   File: ``unit/test_logging.c``

.. literalinclude:: ../../tests/unit/test_logging.c
   :language: c
   :lines: 111-151
   :caption: tests/unit/test_logging.c (test_logging_concurrency)


Test Read CSV - Case Success
---------------------------------------
.. _test_read_csv_success:

.. c:function:: static void test_read_csv_success(void)

   Implements tests for :ref:`SWR2.1` and :ref:`SWR4.2`.
   
   This function tests if the csv file 
   with the sensors readings is correctly loaded by the system.

   File: ``unit/test_bcm.c``

.. literalinclude:: ../../tests/unit/test_bcm.c
   :language: c
   :lines: 160-169
   :caption: tests/unit/test_bcm.c (test_read_csv_success)


Check Disable Engine - Case All OK
-----------------------------------
.. _test_check_disable_engine_all_ok:

.. c:function:: static void test_check_disable_engine_all_ok(void)

   Implements tests for :ref:`SWR2.2`, :ref:`SWR2.3`, :ref:`SWR2.4`, :ref:`SWR2.6`, :ref:`SWR2.7`, :ref:`SWR2.9`, :ref:`SWR4.3`, :ref:`SWR4.4`, and :ref:`SWR5.1`.
   
   This function tests engine disable conditions based on sensor readings
   such as speed, acceleration, brake status, and temperature.

   It logs system messages and CAN errors according to defined failure modes.

   File: ``unit/test_powertrain.c``

.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 149-171
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_all_ok)


Test Check Disable Engine - Fail Cond1
---------------------------------------
.. _test_check_disable_engine_fail_cond1:

.. c:function:: static void test_check_disable_engine_fail_cond1(void)

   Implements tests for :ref:`SWR2.8`.

   This function tests engine disable conditions based on sensor readings
   of the brake fail and there is no acceleration.

   It logs system messages and CAN errors triggered by brake failure conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 180-215
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond1)

Test Check Disable Engine - Fail Cond2
---------------------------------------
.. _test_check_disable_engine_fail_cond2:

.. c:function:: static void test_check_disable_engine_fail_cond2(void)

   Implements tests for :ref:`SWR2.6` and :ref:`SWR2.8`.

   This function tests engine disable conditions based on sensor readings
   of the engine temperature below the minimum set.

   It logs system messages and CAN errors triggered by low engine temperature failure conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 225-259
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond2)

Test Check Disable Engine - Fail Cond3
---------------------------------------
.. _test_check_disable_engine_fail_cond3_inactive:

.. c:function:: static void test_check_disable_engine_fail_cond3_inactive(void)

   Implements tests for :ref:`SWR2.2` and :ref:`SWR2.8`.

   This function tests engine disable conditions based on sensor readings
   of the brake fail and there is no acceleration.

   It logs system messages and CAN errors triggered by brake failure conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 269-303
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond3_inactive)

Test Check Disable Engine - Fail Cond4
---------------------------------------
.. _test_check_disable_engine_fail_cond4:

.. c:function:: static void test_check_disable_engine_fail_cond4(void)

   Implements tests for :ref:`SWR2.3`, :ref:`SWR2.8`, :ref:`SWR4.3` and :ref:`SWR4.4`.

   This function tests engine disable conditions based on sensor readings
   of the battery voltage and SoC below the minimum set.

   It logs system messages and CAN errors triggered by low battery conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 315-350
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond4)

Test Check Disable Engine - Fail Cond5
---------------------------------------
.. _test_check_disable_engine_fail_cond5:

.. c:function:: static void test_check_disable_engine_fail_cond5(void)

   Implements tests for :ref:`SWR2.7` and :ref:`SWR2.8`.

   This function tests engine disable conditions based on sensor readings
   of the doors.

   It logs system messages and CAN errors triggered by open door conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 360-394
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond5)

Test Check Disable Engine - Fail Cond6
---------------------------------------
.. _test_check_disable_engine_fail_cond6:

.. c:function:: static void test_check_disable_engine_fail_cond6(void)

   Implements tests for :ref:`SWR2.8` and :ref:`SWR2.9`.

   This function tests engine disable conditions based on sensor readings
   of the tilt angle above the threshold.

   It logs system messages and CAN errors triggered by tilt angle conditions.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 404-438
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_fail_cond6)

Test Handle Engine Restart
-----------------------------------
.. _test_handle_engine_restart:

.. c:function:: static void test_handle_engine_restart(void)

   Implements tests for :ref:`SWR2.5`, :ref:`SWR3.1`, :ref:`SWR3.4` and :ref:`SWR3.5`.

   This function test if engine will restart successfully if sensors
   read are inside the threshold.

   It logs system messages and handle engine restart properly.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 466-537
   :caption: tests/unit/test_powertrain.c (test_handle_engine_restart)

Test Function Start Stop
------------------------
.. _test_function_start_stop:

.. c:function:: static void test_function_start_stop(void)

   Implements tests for :ref:`SWR1.2`.

   This function tests if when the driver activates stop/start 
   the engine turn off successfully.

   It logs system messages and handle engine restart properly.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 605-636
   :caption: tests/unit/test_powertrain.c (test_function_start_stop)


Test Parse Input Variants Powertrain
-------------------------------------
.. _test_parse_input_variants_pw:

.. c:function:: static void test_parse_input_variants_pw(void)

   Implements tests for :ref:`SWR1.2`.

   This function tests the parsing of various input strings to ensure
   the system correctly interprets and updates internal states 
   based on received sensor data.

   It logs system messages and handles input parsing properly.

   File: ``unit/test_powertrain.c``
.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 645-708
   :caption: tests/unit/test_powertrain.c (test_parse_input_variants_pw)

Test Process Received Frame
---------------------------
.. _test_process_received_frame:

.. c:function:: void test_process_received_frame(void)

   Implements tests for :ref:`SWR1.2` and :ref:`SWR1.4`.

   This function tests the behavior of process received frame when handling
   valid and invalid frames, ensuring proper logging and system state changes.

   File: ``unit/test_dashboard.c``
.. literalinclude:: ../../tests/unit/test_dashboard.c
   :language: c
   :lines: 78-101
   :caption: tests/unit/test_dashboard.c (test_process_received_frame)

Test Parse Input Variants
-------------------------
.. _test_parse_input_variants_dashboard:

.. c:function:: void test_parse_input_variants(void)

   Implements tests for :ref:`SWR1.3`, :ref:`SWR4.5`, :ref:`SWR5.1`, :ref:`SWR5.2`, :ref:`SWR5.3` and :ref:`SWR6.3`.

   This function tests the parsing of various input strings to ensure
   the dashboard correctly interprets and updates internal states
   based on received commands and sensor data.

   File: ``unit/test_dashboard.c``
.. literalinclude:: ../../tests/unit/test_dashboard.c
   :language: c
   :lines: 116-216
   :caption: tests/unit/test_dashboard.c (test_parse_input_variants)

Test Send Encrypted Message
----------------------------
.. _test_send_encrypted_message:

.. c:function:: void test_send_encrypted_message(void)

   Implements tests for :ref:`SWR1.4`.

   This function tests if a frame was received 
   by the dashboard and triggers the activation 
   of the start_stop_active variable.

   File: ``unit/test_can_socket.c``
.. literalinclude:: ../../tests/unit/test_can_socket.c
   :language: c
   :lines: 195-212
   :caption: tests/unit/test_can_socket.c (test_send_encrypted_message)

Test Check Health Signals - Immediate
---------------------------------------
.. _test_check_health_signals_immediate:

.. c:function:: void test_check_health_signals_immediate(void)

   Implements tests for :ref:`SWR6.1`.

   This function tests a fault simulation and check health signals to identify the fault immedeatly.

   File: ``unit/test_bcm.c``
.. literalinclude:: ../../tests/unit/test_bcm.c
   :language: c
   :lines: 557-582
   :caption: tests/unit/test_bcm.c (test_check_health_signals_immediate)

Test Check Health Signals - Persisted
---------------------------------------
.. _test_check_health_signals_persisted:

.. c:function:: void test_check_health_signals_persisted(void)

   Implements tests for :ref:`SWR6.1`, :ref:`SWR6.2`, :ref:`SWR6.3`, and :ref:`SWR6.4`.

   This function tests a fault simulation in the tilt angle
   and check health signals, and after elapsing the safety time
   send the system disabled warning.

   File: ``unit/test_bcm.c``
.. literalinclude:: ../../tests/unit/test_bcm.c
   :language: c
   :lines: 593-645
   :caption: tests/unit/test_bcm.c (test_check_health_signals_persisted)

Test Check Health Signals - Engine Temperature
-----------------------------------------------
.. _test_check_health_signals_engine_temp:

.. c:function:: void test_check_health_signals_engine_temp(void)

   Implements tests for :ref:`SWR6.3` and :ref:`SWR6.4`.

   This function tests a fault simulation in the engine temperature
   and check health signals, and after elapsing
   the safety time send the system disabled warning.

   File: ``unit/test_bcm.c``
.. literalinclude:: ../../tests/unit/test_bcm.c
   :language: c
   :lines: 654-706
   :caption: tests/unit/test_bcm.c (test_check_health_signals_engine_temp)

Test Check Health Signals - Door Status
------------------------------------------
.. _test_check_health_signals_door_status:

.. c:function:: void test_check_health_signals_door_status(void)

   Implements tests for :ref:`SWR6.3` and :ref:`SWR6.4`.

   This function tests a fault simulation in the door status
   and check health signals, and after elapsing the safety time
   send the system disabled warning.

   File: ``unit/test_bcm.c``
.. literalinclude:: ../../tests/unit/test_bcm.c
   :language: c
   :lines: 715-767
   :caption: tests/unit/test_bcm.c (test_check_health_signals_door_status)
