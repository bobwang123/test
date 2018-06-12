.PHONY: all clean
PYTHON:=python

all: setup-cost setup-task setup-scheduler

setup-cost: setup-cost.py
	$(PYTHON) $< build_ext --inplace

setup-task: setup-task.py
	$(PYTHON) $< build_ext --inplace

setup-scheduler: setup-scheduler.py
	$(PYTHON) $< build_ext --inplace

clean:
	$(RM) *.c *.so
	$(RM) -r build/ __pycache__/
