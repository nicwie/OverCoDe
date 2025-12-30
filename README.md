# OverCoDe

## Build Instructions

1. Create a build directory:

   ```bash
   mkdir build
   ```

2. Configure the project:

   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

   - To enable code coverage: `cmake -DENABLE_COVERAGE=ON ..`

3. Build the executables:

   ```bash
   cmake --build build
   ```

## Experiment Commands

### Experiment 1

Run:

```sh
    ./build/OverCoDe false 0.92 0.85 result.txt 20 20 112 37 12
```

### Experiment 2

```bash
    ./build/OverCoDe true 0.92 0.85 result.txt 200 20
```

### Verify output

```bash
    ./compareToTruth.py result.txt result.txt_truth <graphs> <runs>
```
