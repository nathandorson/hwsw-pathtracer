# hwsw-pathtracer

Monte Carlo path tracers are programs that perform three-dimensional scene
rendering by simulating bouncing rays of light for a random sample of rays in the
scene. With proper modeling of surface behavior and light sources in the scene,
impressively realistic renders can be achieved given sufficient compute.

The aim of this project is to implement a path tracer that runs in software on the
processing system (PS) of a Zynq7020 SoC, and then shift the actual tracing
algorithm onto the programmable logic (PL) of the system to explore the possibility
of speeding up the tracing via greatly increased parallelism.

## Software implementation

The initial software implementation consists of a single python module in the
`pathtracer.py` file, which contains all the logic for performing basic
path tracing.

The core of the path tracing application is the procedure for tracing a single ray
of light throughout the scene and calculating what color that ray is (as seen by
the camera in the scene). In the software implementation, this is fully functional,
and rays are made to bounce off of scene objects (via intersection calculations)
until they reach a light source, or hit the bounce depth limit. They accumulate
color as they bounce, and return the result to be used in calculating the color
for the associated pixel in the final render. This procedure is repeated for as
many rays as necessary to calculate the colors of all of the pixels in the render.

The capabilities of the basic software pathtracer include:
- loading scenes to render from a JSON file
- supporting diffuse sphere and plane primitives as scene objects
- configurable camera settings
- configurable number of bounces / number of rays fired per render pixel
- outputting final renders to files on disk

### Running the software implementation

Put the contents of the `python/` directory somewhere on
the filesystem of the device you would like to run on.
Then navigate to that location in a terminal and invoke
the test script with `python3 ./run.py`

This will result in the scene render being saved on disk to
`test_render_result.png`. You can open this however you like to view it.
Or, if you put the code below in a jupyter notebook, and run it, then
the image will additionally render in the notebook for your convenience.

Alternatively, input the following code into a jupyter notebook,
or an interactive python shell and run in that way.

```python
from pathtracer import *
import matplotlib.pyplot as plt

p = Pathtracer()
p.rays_per_pixel = 1     # This can be increased to increase quality.
p.depth = 4              # This can be increased to increase quality.
p.load_from_file("scenes/cornell_box.json")
p.render_scene()

plt.imshow(p.usable_pixel_array())
plt.show()

p.save_rendered_scene('test_render_result.png')
```

## Hardware implementation

As of 2024-03-29 the hardware implementation via Vitis HLS is still in progress. There is a progress update in [UPDATE.md](./UPDATE.md)

## Project structure

```
.
├── assets
│   ├── cornell_high_def.png
│   ├── cornell_low_def.png
│   ├── pynq_run_sample.png
│   └── ...
├── hls                         # Initial attempt at HLS code
│   └── ...
├── hls-v2                      # Second, better attempt at HLS
│   └── ...
├── python                      # Python implementation
│   ├── pathtracer.py             # The main software implementation file
│   ├── run.py                    # Sample script to run the pathtracer
│   └── scenes                    # Scenes that the pathtracer can render
│       ├── box_simple.json         # Simple box scene
│       ├── cornell_box.json        # Cornell box scene
│       └── light_plane.json        # Scene with a single light source
└── README.md
```
## Sample results

Note that the first two results are captured on a machine with a decent CPU, and
still take a little while to render. On the PS of the PYNQ board, such renders take
far longer.

#### Rendering a basic Cornell box scene with two spheres in the scene
![](./assets/cornell_low_def.png)

#### Rendering the same scene with a higher ray count
![](./assets/cornell_high_def.png)

#### Running on the XILINX PYNQ setup

Note the running time here - over 15 minutes for the worst possible render.
Not great! We will aim to improve that via hardware acceleration and
possibly some software tricks as well. Also note that this is running on a scene
with lower exposure, thus the dark scene. If you run it yourself you will get a
brighter result due to the scene having been updated since this run was performed.

![](./assets/pynq_run_sample.png)
