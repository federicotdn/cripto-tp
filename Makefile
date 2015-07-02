# Makefile cripto 2015

CC = gcc
CFLAGS += -MD -MP -Wall -g
LDFLAGS += -lm
SRC = $(wildcard src/*.c)
TARGET = bin/cripto
RM = rm -f

$(TARGET): $(SRC:%.c=%.o)
	$(CC) -o $@ $^ $(LDFLAGS)

-include $(SRC:%.c=%.d)

clean:
	@echo "Cleaning..."
	@$(RM) src/*.o src/*.d
	@$(RM) $(TARGET)
	@echo "Done."