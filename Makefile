ifdef VIRTUAL_ENV
    PYTHON := $(VIRTUAL_ENV)/bin/python
	PIP := $(VIRTUAL_ENV)/bin/pip
else
    PYTHON := $(shell which python3 2>/dev/null || which python 2>/dev/null || echo python3)
	PIP := $(shell which pip3 2>/dev/null || which pip 2>/dev/null || echo pip3)
endif
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I/opt/homebrew/include
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
	@cd mini_requests_rust && maturin develop -r --pip-path $(PIP)

beast_boost:
	@cd mini_requests_cpp && $(MAKE) install-wheel

print_header:
	@printf "%10s\t%10s\t%8s\t%8s\t%8s\t%8s\n" "Language" "Library" "Mean(ms)" "P99(ms)" "P999(ms)" "Max(ms)"

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


.PHONY: clean run rust_server rust_client print_header