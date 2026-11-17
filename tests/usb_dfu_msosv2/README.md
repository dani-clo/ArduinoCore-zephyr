# USB DFU Microsoft OS 2.0 descriptors

Run the descriptor tests with the workspace's Zephyr environment active:

```sh
west build -b native_sim/native/64 -d build/test-usb-dfu-msosv2 tests/usb_dfu_msosv2
build/test-usb-dfu-msosv2/zephyr/zephyr.exe
```

The virtual USB controller tests cover the composite runtime descriptor, the
standalone DFU descriptor, different FS/HS interface assignments, partial reads,
invalid requests, and the UTF-16 `DeviceInterfaceGUIDs` registry property.

`CONFIG_ARDUINO_USB_DFU_MSOSV2` enables the descriptors in the loader and runtime.
The Nano XL Link variant also enables it separately in MCUboot. Each USB context
advertises USB 2.01, a USB 2.0 Extension capability, and the Microsoft OS 2.0
Platform Capability. Vendor request `0x02` is reserved for the descriptor set
(`bmRequestType=0xC0`, `wIndex=0x0007`, `wValue=0`).

The descriptor set covers one configuration per speed. Composite runtime uses
the actual interface bitmap assigned to `dfu_runtime` by Zephyr. Standalone DFU
uses device-level features. Both expose compatible ID `WINUSB` and the stable
interface GUID `{B26A9C77-62D3-4DA3-9D91-728C6B7ABF81}`.

## Windows hardware validation

Use Windows 10 or 11 with the updated MCUboot and loader/sketch images. Start
with a machine without previous driver associations for the board, or remove
the old device/driver associations before testing automatic installation.

1. In runtime (`2341:007e`), verify the CDC COM port still uses `usbser.sys` and
   only the DFU runtime function uses `winusb.sys`.
2. In USBView, check `bcdUSB=0x0201`, both BOS capabilities, the Microsoft
   descriptor-set length, and the DFU function's actual interface number.
3. Use the normal Arduino upload recipe to exercise DFU DETACH, re-enumeration
   into loader DFU (`2341:007e`), and download through `dfu-util` without Zadig
   or a custom INF. Verify that the board returns to runtime with a working COM
   port and that a second upload also succeeds.
4. Double-tap reset to enter MCUboot DFU (`2341:017e`). Verify automatic WinUSB
   installation, DFU discovery, and recovery download.
5. Repeat with FS and HS connections. Check transitions carefully because the
   runtime composite device and standalone loader DFU share a VID/PID and serial.

Microsoft OS 2.0 descriptors support Windows 8.1 and later. The virtual tests
do not verify Windows driver selection or USB transfers on physical hardware.
