PYTHON = $(VIRTUAL_ENV)/bin/python
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -ffast-math -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib
LIBS = -lboost_system -pthread
TARGET = bench
SOURCE = main.cpp

$(TARGET): $(SOURCE)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

rust_client:
	@cargo build --release -p rust_client

rust_server:
	@cargo build --release -p rust_server

reqwest_pyo3:
	@cd mini_requests_rust && maturin develop -r

beast_boost:
	@cd mini_requests_cpp && $(MAKE) install-wheel

print_header:
	@printf "%10s\t%10s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n" "Language" "Library" "Mean(ms)" "P99(ms)" "P999(ms)" "Max(ms)" "LMean(ms)" "Bench(ms)"

run_cpp: rust_server $(TARGET) print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		./$(TARGET); \
	'

run_rust: rust_server rust_client print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		./target/release/rust_client; \
	'

run_python: rust_server reqwest_pyo3 beast_boost print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		$(PYTHON) main.py; \
	'

run_without_build: print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		./$(TARGET); \
		./target/release/rust_client; \
		$(PYTHON) main.py; \
	'

run: $(TARGET) rust_server rust_client reqwest_pyo3 beast_boost run_without_build

clean:
	rm -f $(TARGET)
	cargo clean
	cd mini_requests_cpp && $(MAKE) clean



check-and-install-deps:
	@echo "Checking and installing dependencies..."
	@echo "Checking Rust installation..."
	@command -v cargo >/dev/null 2>&1 || { echo >&2 "Cargo is required but not installed. Aborting."; exit 1; }
	@echo "Checking Python venv activation..."
	@if [ -z "$(VIRTUAL_ENV)" ]; then \
		echo "No virtual environment activated. Please activate your Python virtual environment."; \
		exit 1; \
	else \
		echo "Virtual environment is activated: $(VIRTUAL_ENV)"; \
	fi
	@echo "Checking g++ installation..."
	@command -v g++ >/dev/null 2>&1 || { echo >&2 "g++ is required but not installed. Aborting."; exit 1; }
	@echo "Checking and installing Boost libraries..."
	@if [ "$(shell uname)" = "Darwin" ]; then \
		if ! brew list boost >/dev/null 2>&1; then \
			echo "Boost not found, installing via Homebrew..."; \
			brew install boost boost-python3; \
		else \
			echo "Boost is already installed."; \
		fi; \
	elif [ "$(shell uname)" = "Linux" ]; then \
		if ! dpkg -l | grep -q libboost-all-dev; then \
			echo "Boost not found, installing via apt-get..."; \
			sudo apt-get update && sudo apt-get install -y libboost-all-dev; \
		else \
			echo "Boost is already installed."; \
		fi; \
	else \
		echo "Unsupported OS. Please install Boost manually."; exit 1; \
	fi
	@echo "Checking and installing python dependencies..."
	@$(PYTHON) -m pip show maturin > /dev/null 2>&1 || { echo "Installing maturin..."; $(PYTHON) -m pip install maturin; }
	@$(PYTHON) -m pip show requests > /dev/null 2>&1 || { echo "Installing requests..."; $(PYTHON) -m pip install requests; }
	@$(PYTHON) -m pip show aiohttp > /dev/null 2>&1 || { echo "Installing aiohttp..."; $(PYTHON) -m pip install aiohttp; }
	@$(PYTHON) -m pip show build > /dev/null 2>&1 || { echo "Installing build..."; $(PYTHON) -m pip install build; }
	@echo "All dependencies are installed."

.PHONY: clean run rust_server rust_client print_header