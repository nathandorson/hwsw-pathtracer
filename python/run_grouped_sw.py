from pathtracer import *
import time

p = Pathtracer()
p.rays_per_pixel = 1
p.depth = 3
p.load_from_file("scenes/cornell_box.json")
t1 = time.time()
p.render_scene_in_software()
t2 = time.time()
print(f'Took {t2-t1} seconds to render scene, running {p.iters} iterations')

p.save_rendered_scene('test_render_result.png')
