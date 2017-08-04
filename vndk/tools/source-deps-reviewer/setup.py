from setuptools import setup, find_packages

setup(
    name='sourcedr',
    version='1.0',
    description='Shared Libs Deps Review Tool',
    url='https://android.googlesource.com/platform/development/+/master/vndk/tools/source-deps-reviewer/',
    packages=['sourcedr'],
    install_requires=['flask'],
    extras_require={
        'dev': [
            'flask_testing'
        ],
    },
    entry_points={
        'console_scripts': [
            'sourcedr = sourcedr.server:main',
        ],
    }
)
