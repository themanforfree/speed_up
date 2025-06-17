CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I/opt/homebrew/include
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

run_python: rust_server reqwest_pyo3 print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		python3 main.py; \
	'

run: $(TARGET) rust_server rust_client reqwest_pyo3 print_header
	@bash -c '\
		./target/release/rust_server & \
		SERVER_PID=$$!; \
		trap "kill $$SERVER_PID" EXIT; \
		sleep 0.5; \
		./$(TARGET); \
		./target/release/rust_client; \
		python3 main.py; \
	'

clean:
	rm -f $(TARGET)
	cargo clean


.PHONY: clean run rust_server rust_client print_header