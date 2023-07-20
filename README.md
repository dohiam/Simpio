# Simpio: A PIO Simulator

copyright David Hamilton 2023

## Overview

The Raspberry Pi Pico microcontroller is a chip developed by the Raspberry Pi foundation and supports a Programmable IO instruction-set that runs in custom hardware called PIO. The Pico SDK does not provide debugging support for this instruction-set which makes PIO software somewhat difficult to develop. A few simulators exist to help with this, and Simpio is one of these. 

Simpio is focused on **education** rather than professional development:

- It is completely standalone: 
  It includes an editor, compiler, run-time environment, and debugger so that PIO programs can be developed, run, and debugged without requiring any additional hardware or any additional software. It does not depend on the Pico SDK and does not require any C programming language development. It allows one to focus only on the PIO assembly language (which presumably is what one is trying to learn how to use).
- It is easy to run:
  A small (less than 2MB) static-built executable is available that should run on any Posix/Linux terminal, including the Windows Subsystem for Linux (WSL) in Windows 10 and 11. It doesn't require any "installation" and does not depend on any additional packages or run-time libraries. It doesn't depend on any graphical user interface. It only needs a command line to run and yet it provides a full screen user interface (using Ncurses). There is even a 32 bit binary provided for the ultimate in portability.
  It is also easy to build Simpio, requiring only make, Ncurses, and Lexx+Yacc to build.
- It is very visual:
  The user interface shows the state of all PIO hardware elements, highlighting what has changed as a result of executing the last statement or since the last breakpoint. In single step mode, this helps one understand the behavior of each instruction as it executes. It also allows one to see how program flow moves from one instruction to another, even when multiple PIO and user programs are executing simultaneously (by interleaving instructions from all executing processors in a round-robin fashion). 
  There is also a feature to show a visual timeline of selected GPIO pins. Those familiar with IO protocol timing diagrams showing high/low states over time will recognize these diagrams; for others, the documentation explains them.
- It includes sample PIO program examples and a (brief) PIO tutorial:
  In addition to documentation on how to use Simpio, there is also documentation explaining PIO instructions along with examples that can be stepped-through to understand instructions and typical programming practices before actually trying to write a PIO program. This documentation supplements the existing (excellent) Pico SDK and hardware references by explaining why various features and instructions work the way they do, making follow-on study of the SDK and hardware references a bit easier. (Starting with the SDK and hardware references to learn how to write PIO programs is a bit like studying a dictionary in order to learn how to write a novel.)
- It includes simulated hardware devices and user programs.
  One can write PIO programs that communicate with each other, with user programs, and with peripheral devices, and can visually watch the execution of all these to observe how they interact with each other. Unlike real hardware, where the inner workings of peripheral devices is hidden, the complete inner state of simulated devices can be displayed at any time in Simpio. 
- It does **not** support "real" development:
  Because Simpio simulates user programs by providing some additional statements that are included along with PIO instructions, eliminating the need to write a program in C that would execute in a separate processor to configure, load, and start PIO programs, it doesn't help with developing or debugging those C programs. Once a PIO program is working, moving it to real hardware requires additional software development (and debugging) outside the Simpio environment. Also, since Simpio provides its own compiler and execution environment, there can be differences between program behavior in Simpio vs real hardware. This is why Simpio is not as suitable for ("real") professional development as other simulators. Simpio is focused on learning PIO programming, and also learning simple communications protocols & how they are used to interface with peripheral devices. If one already knows embedded programming and just wants to learn the new PIO instruction set before diving into Pico software development, or wants to learn about simple bit-bang type protocols,  or just wants to do a quick prototype, Simpio is a good choice. Once one knows how to write the PIO program they want to deploy in a real system, a different simulator would be a better choice.
- Although Simpio does not directly support "real" development, it does provide a bridge to working with real hardware through documentation and a python script which translates a PIO program targeted for the Simpio simulator to a PIO program (plus a C main program and a Cmake build file) that allows the same program to be built for real hardware. That is, Simpio "jump starts" someone who has been using the simulator into using real hardware. (See the pio_programming.md document for more on this).

In summary, Simpio provides a complete standalone visual environment for learning PIO programming that runs on any Linux command line - no installation, configuration, or hardware needed. 

The current state of Simpio is "beta". It works but in addition to still lacking certain features, it is not yet thoroughly tested (beyond the test cases in the tests folder). 

See 

[design.md]: design.md

 for information on the structure of the code and dependencies.

[pio_programming.md]: pio_programming.md

 for more on using Simpio, examples, and PIO programming in general.

## 