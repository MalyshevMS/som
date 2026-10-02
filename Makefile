BUILD_DIR = build
TARGET_NAME = som
TARGET_BIN = $(BUILD_DIR)/$(TARGET_NAME)
CMAKE_GENERATOR = Ninja

.PHONY: all build run clean

all: build

build:
	@cmake -B $(BUILD_DIR) -S . -G "$(CMAKE_GENERATOR)"
	@cmake --build $(BUILD_DIR)

run: build
	@if [ -f $(TARGET_BIN) ]; then \
		./$(TARGET_BIN); \
	else \
		echo "Ошибка: Бинарный файл $(TARGET_BIN) не найден!"; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)
