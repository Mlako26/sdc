# Silent Data Corruption

This repository contains all development made during the investigation of fault injections and SDC simulations.

## Repository Structure

- `/doc`: Notes and decisions made along the investigation process.
- `/src/pintool`: Pintools and Makefiles to build them. 
- `/src/tests`: Folder with cpp test files to inject faults into.

## Developing

### Pintools

To create a new pintool at `/src/pintool`, you can copy one of the alreadty existing ones as a starting point. Once it is done, build it with the following command from that directory:

```
make PIN_ROOT=<path to Pin kit> obj-intel64/YourTool.so
```

It can then be ran with any of our test binaries with the following command from that same directory:

```
<path to Pin kit>/pin -t obj-intel64/YourTool.so -- ../tests/YourTest
```

I need to make a makefile to so that test executables are in a separate folder and they can be ignored