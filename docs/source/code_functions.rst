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

.. _test_engine_stop:

test_engine_stop()
------------------
Tests requirement SWR1.2.