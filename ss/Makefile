# Nazwa pliku wynikowego
TARGET = projekt

# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -g

# Lista plików źródłowych
SRCS = main.c proc1.c proc2.c proc3.c ipc.c signals.c

# Lista plików obiektowych (automatycznie generowana)
OBJS = $(SRCS:.c=.o)

# Domyślny cel (to się wykona po wpisaniu 'make')
all: $(TARGET)

# Linkowanie
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Kompilacja poszczególnych plików .c do .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Sprzątanie (wpisz 'make clean' żeby usunąć binarki)
clean:
	rm -f $(OBJS) $(TARGET)

# Cel pomocniczy do szybkiego rekompilowania i uruchamiania
run: all
	@echo "--- [AUTO-CLEAN] Czyszczenie starych procesów i zasobów IPC ---"
	# Minus (-) na początku oznacza: "nie przerywaj, jeśli komenda zwróci błąd"
	# (np. jeśli nie ma czego zabijać)
	-killall -9 projekt 2>/dev/null || true
	-ipcrm -a 2>/dev/null || true
	@echo "--- [START] Uruchamianie programu na czysto ---"
	./projekt