CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib
LIBS = -lcurl -lcpr -lboost_system -pthread
TARGET = bench
SOURCE = main.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

rust_build:
	cargo build --release

print_header:
	@printf "%10s\t%10s\t%8s\t%8s\t%8s\t%8s\n" "Language" "Library" "Mean(ms)" "P99(ms)" "P999(ms)" "Max(ms)"

run_cpp: $(TARGET) print_header
	@./$(TARGET)

run_rust: rust_build print_header
	@./target/release/speed_up

run_python: print_header
	@python main.py

run: $(TARGET) rust_build print_header 
	@./$(TARGET)
	@./target/release/speed_up
	@python main.py

clean:
	rm -f $(TARGET)
	cargo clean


.PHONY: clean run
