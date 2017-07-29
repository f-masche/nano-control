build:
	platformio run

upload:
	platformio run --target upload

monitor:
	platformio serialports monitor
