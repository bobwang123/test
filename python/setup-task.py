from distutils.core import setup
from distutils.extension import Extension
# noinspection PyPackageRequirements
from Cython.Distutils import build_ext
ext_modules = [
        Extension("task", ["task.py"]),
        ]
setup(
        name='Zhicang logistics package',
        cmdclass={'build_ext': build_ext},
        ext_modules=ext_modules
        )
