
all: pingit

pingit: pingit.c
	gcc \
		-Wall -g \
		-o pingit \
		pingit.c \
		$(shell pkg-config --cflags --libs alsa) \
		-lrt -lm

