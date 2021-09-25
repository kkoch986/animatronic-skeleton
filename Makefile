PIO=${HOME}/.local/bin/pio
ENVSTR=
ifdef PIOENV
	ENVSTR="-e"${PIOENV}
endif

compile: 
	${PIO} run ${ENVSTR}

upload: 
	${PIO} run --target=upload ${ENVSTR}

logs: 
	${PIO} device monitor -b 57600 ${ENVSTR}

.PHONY: compile upload logs
