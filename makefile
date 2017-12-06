CC	= gcc
OBJS	= main.o 
PROG	= urgency

# GENERIC 

all:	${PROG}

clean:
	rm ${OBJS} ${PROG}

${PROG}:	${OBJS}
		${CC} -Wall -pthread ${OBJS} -o $@

.c.o:
		${CC} -Wall -pthread $< -c -o $@

##############################

main.o:	main.c 