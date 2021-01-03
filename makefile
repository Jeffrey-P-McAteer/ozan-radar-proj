
all: pingit

pingit: pingit.c
	gcc \
		-Wall -Werror \
		-o pingit \
		$(shell pkg-config --cflags --libs alsa) \
		pingit.c

