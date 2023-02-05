# py_ftdi
Wrapper around ftdi library to communicate with FTDI connected devices. Works also for synchronous mode.

## list of py_ftdi functions:
- py_ftdi.list_devices() - returns list of connected devices

## list of Device functions:
- `list_devices() -> List[str]`    ...  list connected devices
- `open(dev_name: str, baud: int) -> int`   ... open device, if baud rate specified, open in serial mode
- `close() -> int`   ... close openened device
- `set_sync_mode(is_sync_mode: bool) -> int`   ... set synchronous or asynchronous mode
- `is_connected() -> bool`   ... if device is connected
- `clear_buffers() -> int`   ... clear rx and tx buffers
- `send(data: List[int]) -> int`   ... sends bytes to device (list of ints)
- `read(size: int, timeout: float) -> Tuple[rc, List[int]]`   ... try reads specified number of bytes with timeout


## Example Usage
```python
import py_ftdi

devices = py_ftdi.list_devices()
# ["My FTDI Device"]

device = py_ftdi.Device()

# open(device_name, baud_rate)
# if baud_rate = 0, serial mode not used
rc = device.open(devices[0], 0)
# rc = 0

# sets sync mode (1) or async mode (0)
rc = device.set_sync_mode(1)
# rc = 0

# send 3 bytes 1, 2, 3
rc = device.send([0x1, 0x2, 0x3])
# rc = 3 number of sent bytes

# read 5 bytes, with timeout 1s
rc, data = device.read(5, 1)
# rc = 5, data = [1, 2, 3, 4, 5]

rc = device.close()
# rc = 0
```

## Installation from github with pip
```bash
 pip install git+https://github.com/datoux/py_ftdi
```
