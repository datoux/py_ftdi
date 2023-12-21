import py_ftdi

devs = py_ftdi.list_devices()
print(devs)

dev = py_ftdi.Device()
rc = dev.open(devs[0], 0, 0)
print(rc)

rc = dev.set_sync_mode(1)
print(rc)

rc = dev.send([0x55, 0x1, 0x00, 0x00, 0xAA])
print(rc)
rc = dev.read(2, 1)
print(rc)

rc = dev.send([0x55, 0x2, 0x00, 0x00, 0xAA])
print(rc)
rc = dev.read(2, 1)
print(rc)

rc = dev.close()
print(rc)