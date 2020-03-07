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

In the decades since, computers and the languages we use to program
them have become much more sophisticated.  Gone are the days 
of programming at the lowest level the machine has to offer. Well,
almost.  Assembly language programming is still around, but it's 
used mostly for embeded systems, device drivers, and real time
systems.  And even then, C is still available for most of those
tasks.

So about ten years ago, I began longing for those days of 
assembly programming.  I wanted to take complex tasks and break
them down to their most rudimentery steps, type sequences of
instructions to move a single byte from one place to another, 
increment and decrement, shift bits left or right, and push
and pull them to and from a stack.  I wanted to affect bits
in the status register and branch according which bits
were set or cleared.  I wanted to write clever routines that
do really complex things in as few instructions as posible,
and go over them and over them to *bum out* as many instructions
as possible.

Problem was, I did not have any of the old equipment to expiriment
on.  To that end, I set about writing a CPU emulator.  The first
one I tried to write was a CPU of my own design, with an instruction
set, register layout, and control system I dreamed up.  But it
sucked.  There was no already written code that could run on it. And
the instructions were really inconsistant with a clean system.

It occured to me then that maybe I shouldn't try to make my own
CPU, but to emulate ones that were already a thing, and that had 
software already written for them.  But I wasn't sure which one
to try.  I sat on the idea for a long time.  Then I saw a release
of the code Steve "Woz" Wozniak had written for the Disk II boot
sector for the Apple II line of computers.  It was written for the
**MOS Technology 6502** CPU, and I had a lot of exposure to the Apple
II computers as a kid, and had even done a little bit of assembly 
on it.

So, the **6502** was my choice.  I sat on the idea for a long time 
more, revisiting it when I was bored or otherwise unocupied.  In my
mind, I designed, redesigned, and redesigned how such code would 
work.  Finally I sat down at my modern computer and began writting 
it in C.  Before long I had _most_ of the instructions implemented,
and had found a bit of test code to check and make sure they worked
correctly.

Once I'd taken that code as far as I could at that point, I decided
the ultimate test was to see if I could get and Apple II ROM binary
to work on it.  But first I had to study up on how the Apple II worked,
as the test would only be valid if I can output to a video emulator
and input from the keyboard.  So I created a text video emulator using
the SDL graphics library and a download of the Apple II character 
generator ROM.

Then I plugged in an Apple II ROM, and fired it up, and it immediately
crashed.  I had more 6502 instructions to emulate.  Fired it up again, 
got **APPLE ][** at the top of the screen, and then crashed.  Couple more
rounds of fixing instruction that did not work quite right, fired it
up again, got **APPLE II** at the top, and a prompt with a blinking 
cursor!  I typed in the old "Hello, World!" program, and ran it, and it 
said "Hello, World!" right back!

```
               APPLE ][

]100 PRINT "HELLO, WORLD!"

]RUN
HELLO, WORLD!

]
```

Once I got the basic Apple II emulator working, I began developement on
the Disk II emulator.  This greately increased the amount of software
available, because now I could go out get download disk images to run
on the emulator.  After that, I emulated the speaker, and I could play
rudimentary music, and even S.A.M. speech synthesis software.  Then I 
improved the video to run hi-res graphics, and I could play all the 
old games I used to play as a kid.  It was almost like having my own
Apple II!

Getting this emulator up and running was a very satisfying project.
I think the coolest thing about it was that I wrote software capable
of running software written by other people a long time ago, software
that I have no idea how it really does what it does, but that *my*
software execute *it*!

I eventually grew tired of working on that emulator.  I decided to
take a long break from it.  But I knew I'd get back to that project
sooner or later.  And I did just that a few weeks ago when I developed
the **Intel 8080** CPU emulator.  Not quite as big a project as the 
**6502** but fun and interesting nontheless.  

I am currently working on what I call the **dynosaur** project.  It's
name comes from the word *dinosaur*, because it's all about ancient
computer hardware, and *dynamic*, because I am wrapping code around 
the **6502** and **8080** CPU libraries to make them into *plugins*
which can be swapped out with each other by the **dynosaur**.  It 
will eventually have a SDL graphics based video output and keyboard
input, as well as probably other devices.  I would like to emulate
other CPUs as well, such as the **Z80** and **TMS9900**, and they
will be able to plug in to the **dynosaur** as well.

### Directories

* **6502_cpu** MOS Technology 6502 CPU emulator
* **8080_cpu** Intell 8080 CPU emulator
* **dynosaur** Emulator tesing platform with a plugable CPU framework
* **include**  Sharable header files from all subdirectories.
* **markII**   Apple II emulator that uses the 6502 CPU emulation
* **shell**    Basic shell library to test and control emulators

