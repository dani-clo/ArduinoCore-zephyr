.. zephyr:code-sample:: usb-dfu
   :name: USB DFU
   :relevant-api: usbd_api usbd_dfu

   Implement a basic USB DFU device

Overview
********

This sample is used in this repository as a USB DFU bootloader for
Arduino Nano Chandler, using the USB device next stack.

Requirements
************

This project requires a board with UDC API support.

Building and Running
********************

Build the DFU bootloader sample:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/dfu
   :board: arduino_nano_chandler
   :goals: build flash
   :compact:

Arduino Nano Chandler: DFU Sample As Bootloader (No MCUboot)
*************************************************************

In this repository, this sample can be used as a temporary bootloader on
Arduino Nano Chandler without MCUboot.

The expected flash layout is:

* ``dfu_boot`` at ``0x0C000000``: contains this DFU sample (bootloader role).
* ``loader`` at ``0x0C080000``: contains the zephyr loader image.
* ``user_sketch`` at ``0x0C0C0000``: contains the user application image.

Boot behavior is:

* On power-up/reset, DFU sample starts from boot flash.
* A double reset keeps the device in DFU mode.
* Without double reset, DFU sample checks if the loader image is present and
   jumps to it.

`dfu-util`_ can be used to download the loader image into ``slot0_image`` and the application image into ``slot1_image``.

Application image requirements for chainload
============================================

The application image must include its vector table at the beginning of
``user_sketch`` (``0x0C0C0000``). On PIC32CK this means the app must not place
``rom_start`` in Boot Flash Memory (0x08000000).

Build and package ``Loader`` and ``Blink``
=================================================

Build and flash the DFU bootloader to the board via JTAG/SWD.

.. code-block:: console

    venv/bin/west build -p -b arduino_nano_chandler samples/dfu_boot

Build and download the Loader to ``slot0_image`` from DFU mode:

.. code-block:: console

    ./extra/build.sh nano_chandler

    dfu-util -d 2341:ffff -a slot0_image -D build/arduino_nano_chandler_pic32ck1025gc01144/zephyr/zephyr.bin

Build and download the Blink sketch to ``slot1_image`` from DFU mode:

.. code-block:: console

    arduino-cli compile ~/Arduino/sketch/Blink/ -b arduino-git:zephyr:nano_chandler -v -e
    
    dfu-util -d 2341:ffff -a slot1_image -D ~/Arduino/sketch/Blink/build/arduino-git.zephyr.nano_chandler/Blink.ino.elf-zsk.bin

After download, reset the board. The DFU sample will chainload the loader image
from ``0x0C080000`` and starts the sketch.


.. _dfu-util: https://dfu-util.sourceforge.net/
