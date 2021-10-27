PIO=${HOME}/.local/bin/pio
BAUDRATE=115200
ENVSTR=
ifdef PIOENV
	ENVSTR="-e"${PIOENV}
endif

compile: 
	${PIO} run ${ENVSTR}

upload: 
	${PIO} run --target=upload ${ENVSTR}

logs: 
	${PIO} device monitor -b ${BAUDRATE} ${ENVSTR}

.PHONY: compile upload logs
