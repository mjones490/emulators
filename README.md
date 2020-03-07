## Emulator Project

This is a project aimed at creating old CPU emulators,
bus emulators, and any devices, such as keyboard and
monitor emulators.

### Backstory

Computers have always fascinated me.  I enjoy programming them,
learning how they work, and learning the history behind them.
I've been around them since the early '80s, when I was a kid,
and "home" computers at the time were mostly simple, 8 bit machines
that hooked up to a TV.  Most of them had under 64K (yes K) of
RAM, simple graphics, simple (if any) sound capabilities, and 
they ran at less than 5mHz. None had hard drives, and only a very 
few had floppy drives. We had to use cassette tapes for long term 
storage.  Those were the days.  Most had some form of built in 
BASIC.

If you were lucky, your machine gave you the capability to peek
and poke bytes directly into RAM.  If you were _really_ lucky, you
had a program called and assembler, which read human readable
instructions and turned them into machine code that the computer
could understand.  This resulting code ran much faster than code
written in BASIC, but was also harder to understand for most
people.  But it was whole lot of fun to work on.

### Directories

* **6502_cpu** MOS Technology 6502 CPU emulator
* **8080_cpu** Intell 8080 CPU emulator
* **dynosaur** Emulator tesing platform with a plugable CPU framework
* **include**  Sharable header files from all subdirectories.
* **markII**   Apple II emulator that uses the 6502 CPU emulation
* **shell**    Basic shell library to test and control emulators

