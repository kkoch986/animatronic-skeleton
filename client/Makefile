PIO=`which pio`
BAUDRATE=115200
ENVSTR=esp12f
ifdef PIOENV
	ENVSTR=${PIOENV}
endif

IP="skeleton-0.local"

compile:
	${PIO} run -e ${ENVSTR}

upload:
	${PIO} run --target=upload -e ${ENVSTR}

upload-ota:
	${PIO} run --target=upload --upload-port=${IP} -e ${ENVSTR}OTA

logs: 
	${PIO} device monitor -b ${BAUDRATE} -e ${ENVSTR}

all: compile

.PHONY: compile upload logs all
