build:
	platformio run

upload:
	platformio run --target upload

monitor:
	platformio serialmonitor -p /dev/cu.usbmodem1411
