
all: pingit

pingit: pingit.c
	gcc \
		-Wall -g \
		-o pingit \
		$(shell pkg-config --cflags --libs alsa) \
		-lrt -lm \
		pingit.c

