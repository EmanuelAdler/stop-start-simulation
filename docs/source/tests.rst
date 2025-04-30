Test Cases
==========

Check Disable Engine - Case All OK
-----------------------------------
.. _test_check_disable_engine_all_ok:

.. c:function:: static void test_check_disable_engine_all_ok(void)

   Implements tests for :ref:`SWR5.1`

   This function tests engine disable conditions based on sensor readings
   such as speed, acceleration, brake status, and temperature.

   It logs system messages and CAN errors according to defined failure modes.

   File: ``unit/test_powertrain.c``

.. literalinclude:: ../../tests/unit/test_powertrain.c
   :language: c
   :lines: 135-163
   :caption: tests/unit/test_powertrain.c (test_check_disable_engine_all_ok)

.. _test_engine_stop:

test_engine_stop()
------------------
Tests requirement SWR1.2.