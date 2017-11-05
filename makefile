CC	= gcc
OBJS	= main.o doctors.o
PROG	= triage

# GENERIC 

all:	${PROG}

clean:
	rm ${OBJS} ${PROG}

${PROG}:	${OBJS}
		${CC} -Wall -pthread ${OBJS} -o $@

.c.o:
		${CC} $< -c -o $@

##############################

main.o:	main.c structs.h

doctors.o: doctors.c doctors.h