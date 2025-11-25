# nTop Core Python Examples

This directory holds self-contained examples of common operations you may want to perform using nTop Core library.

The examples themselves are meant to be fully self-contained in showing how you would utilize nTop Core to perform a commonly-requested task.
This necessitates, however, a somewhat verbose preamble section in which Python-to-C-Library data marshalling and data primitive definitions are established.
It is necessary to understand these sections if you want to use functionality of nTop Core that is not covered in these examples but we recommend starting by skipping to the sections which exercise the library first to get a feel for the library's use. 

## Setup

The examples have been tested to run on [Python 3.11.4](https://www.python.org/downloads/release/python-3114/) but other recent releases should function similarly.

Now create an environment variable to locate the nTop Core library file.

```shell
# Establish the path to the nTop Core library to test if not previously set up
# via the environment variable `NTOP_CORE_LIB`. You can do this per session
# or in a .bashrc or System Environment Variable.

# In windows powershell
$env:NTOP_CORE_LIB = "c:\path\to\ntop_core.dll"
# In *nix
export NTOP_CORE_LIB="/path/to/libntop_core.so.{major}.{minor}.{patch}"
```

We have some common external dependencies in these examples, the specification is stored in [requirements.txt](./requirements.txt)
Install them using your package manager of choice.

```shell
# Using `pip` from this directory
pip install -r requirements.txt
```

## Running the examples

You are now ready to run any of the examples using pattern similar to the following:

```shell
python mesh.py
```