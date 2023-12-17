# Distributed computing course

## ITMO University, 4th year of study, computer science educational program 

### Many thanks to the practitioner of this course, Denis Sergeevich Tarakanov, for his help in mastering the course materials and interesting questions during fascinating defenses

### This repository contains 5 laboratory works on the subject “Distributed Computing”, descriptions of each of the works are provided, as well as instructions for assembling executable files.

### I hope that the presented code can help in mastering the discipline, however, ***I strongly do not recommend cheating on the subject, since the course does have a check for plagiarism.***

## Prerequisites

1. Any linux OS for compiling, build and running

2. Cmake:
```
sudo apt-get update && sudo apt-get install clang
```

3. Clang (minimum 3.5 version is required):
```
sudo apt-get update && sudo apt-get install cmake
```

## Assembling and build

1. Go to folder with this repo;
2. Run:
```
mkdir build
cd build
cmake ..
make
```
All lab works executables will be builded and located at folder ```repo-root/build/src/pa-N```

The same instuction for assembling and build of any lab work and not the entire course as a whole. But folder for creating build sub-folder is folder with interesting lab work source code

