CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
LDFLAGS = -lm

TARGET  = sc

SRCS    = core/star_chart.c core/star_chart_utils.c
OBJDIR	= obj
OBJS    = $(SRCS:core/%.c=$(OBJDIR)/%.o)

# 🧪 Toggle: enable with `make SANITIZE=1`
SANITIZE ?= 0

ifeq ($(SANITIZE),1)
    CFLAGS  += -fsanitize=address -g3
    LDFLAGS += -fsanitize=address
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: core/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

re: clean all

.PHONY: all clean re
