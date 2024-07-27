# coscheduler

Goal of this project to create a scheduler by using c++ coroutines. But why:

# Advatages

- **Replicability**: by using standard c++ language idioms to ceate a task based os less system 
for single cpu systems like microcontrollers.
- **Replicability**: same bahavior on a PC testsystem as on a microcontroller.
- mutexes, sempahores and other task synchronisations stuff is available in interrupt context.


# Disadvateges

It't won't work with multicore CPUs, or exactly: On multicore CPUs it will use only one CPU.

- This is a cooparative tasks approach. Preemtive multitasking is not possible.
- Don't use it if you don't have 100% control of all parts of the software.
- The code is not 100% compatible to existing code. You may have to adopt it. (It's more than switching to FreeRTOS, you have to modify all your sleeping functions)



