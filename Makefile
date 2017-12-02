.PHONY: all clean
PYTHON:=python
all: setup.py
	$(PYTHON) setup.py build_ext --inplace

clean:
	$(RM) *.c *.so
	$(RM) -r build/ __pycache__/
