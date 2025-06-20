# Python environment configuration - dynamically detect active environment
ifdef VIRTUAL_ENV
    PYTHON := $(VIRTUAL_ENV)/bin/python
else
    PYTHON := $(shell which python3 2>/dev/null || which python 2>/dev/null || echo python3)
endif
# Get Python version in format XY (e.g., 311 for Python 3.11)
PY_VERSION = $(shell $(PYTHON) -c "import sys; v=sys.version_info; print(f'{v.major}{v.minor}')")
PY_VERSION_DOT = $(shell $(PYTHON) -c "import sys; v=sys.version_info; print(f'{v.major}.{v.minor}')")

CXX = g++
CXXFLAGS = -std=c++17 -Wall -fPIC -O3 -I/opt/homebrew/include $(shell python3-config --includes)
LDFLAGS = -shared -L/opt/homebrew/lib $(shell python3-config --ldflags) -L$(shell python3-config --prefix)/lib
LIBS = -lboost_python$(PY_VERSION) -lpython$(PY_VERSION_DOT) -lboost_system -pthread
TARGET = dist/mini_requests_cpp.so
SOURCE = lib.cpp

$(TARGET): $(SOURCE)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

clean:
	rm -f $(TARGET) *.o
	rm -rf build dist *.egg-info

wheel:
	$(PYTHON) -m build

install-wheel: wheel
	$(PYTHON) -m pip install --force-reinstall dist/*.whl

install-dev: wheel
	$(PYTHON) -m pip install --force-reinstall --editable .

env-info:
	@echo "Python Environment Information:"
	@echo "================================"
	@echo "Python executable: $(PYTHON)"
	@echo "Pip command: $(PYTHON) -m pip"
	@echo "Python version: $$($(PYTHON) --version)"
	@echo "Virtual environment: $(if $(VIRTUAL_ENV),$(VIRTUAL_ENV),Not in virtual environment)"
	@echo "Python path: $$($(PYTHON) -c 'import sys; print(sys.executable)')"
	@echo "Site packages: $$($(PYTHON) -c 'import site; print(site.getsitepackages())')"

.PHONY: clean wheel install-wheel install-dev test-install env-info
