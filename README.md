# OverCoDe

## Build Instructions

1. Create a build directory:

   ```bash
   mkdir build
   cd build
   ```

2. Configure the project:

   ```bash
   cmake ..
   ```

   - To enable code coverage: `cmake -DENABLE_COVERAGE=ON ..`

3. Build the executables:

   ```bash
   make
   ```
