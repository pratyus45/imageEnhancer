## Face Enhancer (C++14, GCC 6.3.0)

This project batch-enhances face photos by improving sharpness and contrast. It reads images from `images/` and writes enhanced results to `output/`.

### Libraries Used
- stb_image.h: header-only image loader
- stb_image_write.h: header-only image writer

Both headers are from the public-domain `nothings/stb` collection and are included directly (no external linking required).

### Key Techniques
- Unsharp Mask (two passes) to increase perceived sharpness on blurred photos
- Luminance Histogram Equalization to improve global contrast while approximately preserving color

### Why Not OpenCV?
This version avoids OpenCV and uses lightweight, header-only libraries so it works with older compilers and simple builds.

### Requirements
- Windows
- GCC 6.3.0 (MinGW) or compatible C++14 compiler

### Getting the Headers
Place these files next to `imageEnhancer.cpp`:
- `stb_image.h`
- `stb_image_write.h`

You can download them from the `nothings/stb` repository (raw files) and save into the project folder.

### Build
From PowerShell:
```powershell
cd "D:\c++ tutorials\faceEnhancer"
g++ -O2 -std=gnu++14 imageEnhancer.cpp -o enhancer.exe
```

### Run
1) Ensure your input images are in `images/` (supported: .jpg, .jpeg, .png, .bmp)
2) Run the program:
```powershell
./enhancer.exe
```
3) Enhanced images will be written into `output/` with the same filenames

### Folder Structure
- `imageEnhancer.cpp` — main program
- `stb_image.h`, `stb_image_write.h` — header-only libraries
- `images/` — input photos
- `output/` — enhanced results (auto-created)

### Acknowledgements
- `stb_image.h` and `stb_image_write.h` by Sean Barrett and contributors (`nothings/stb`).
