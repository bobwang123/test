from distutils.core import setup
from distutils.extension import Extension
# noinspection PyPackageRequirements
from Cython.Distutils import build_ext
ext_modules = [
        Extension("cost", ["cost.py"]),
        Extension("task", ["task.py"]),
        Extension("scheduler", ["scheduler.py"])]
setup(
        name='Zhicang logistics package',
        cmdclass={'build_ext': build_ext},
        ext_modules=ext_modules
        )
