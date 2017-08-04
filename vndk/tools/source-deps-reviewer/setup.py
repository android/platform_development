from setuptools import setup, find_packages

setup(name='sourcedr',
      version='1.0',
      description='Shared Libs Deps Review Tool',
      url='https://github.com/petwill/source-deps-reviewer',
      author='Fan-Yun Sun',
      author_email='sunfanyun@gmail.com',
      packages=['sourcedr'], 
      install_requires=['flask'],
      extras_require={
          'dev': ['flask_testing'],
      },
      entry_points={
          'console_scripts': [
              'sourcedr = sourcedr.server:main',
          ],
      })
