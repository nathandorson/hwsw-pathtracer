from pathtracer import *
import time

p = Pathtracer()
p.rays_per_pixel = 40
p.depth = 5
p.load_from_file("scenes/cornell_box_tri.json")
t1 = time.time()
p.render_scene()
t2 = time.time()
print(f'Took {t2-t1} seconds to render scene, running {p.iters} iterations')

p.save_rendered_scene('test_render_result.png')
