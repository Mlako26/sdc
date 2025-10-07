# Simulation Algorithm

To figure out which instructions are more SDC prone, we need to implement methods for:

1. Bit flip simulation.
2. SDC recognition of a run program run.
3. Working pipeline that tries with different instructions/registers/bits and compiles the information.

## 1. Bit flip simulation

To emulate fault injections and bit flips, we will use intel's [Pin Instrumentation Tool](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html). We need to take into account different aspects of the problem:

- Do we want to target specific assembly instructions and ignore others? Which?
- Do we want to specify the n'th instruction ran to be injected or should that election be random?
- Do we want to bit flip all bits in a register or just some significant ones? Which would those be?
- How do we defend agains RIP modifications? Should we double it in a second register or something? Stack variables? Is it dangerous?

For now, we will try to generate a summary of the instructions included in a binary, and then perform injections based off of that.

## Implementative Considerations
- Instrumentation code is typically called only once for every instruction, while the analysis code is called everytime the instruction is executed.
- Injection before an instruction should be done only to the registers it uses.
- Just one injection per run.