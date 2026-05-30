CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm

TARGET  = sc

SRCS    = core/star_chart.c core/star_chart_utils.c
OBJS    = $(SRCS:.c=.o)

# 🧪 Toggle: enable with `make SANITIZE=1`
SANITIZE ?= 0

ifeq ($(SANITIZE),1)
    CFLAGS  += -fsanitize=address -g3
    LDFLAGS += -fsanitize=address
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

.PHONY: all clean re