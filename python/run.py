from pathtracer import *

p = Pathtracer()
p.rays_per_pixel = 1
p.depth = 4
p.load_from_file("scenes/cornell_box.json")
p.render_scene()

p.save_rendered_scene('test_render_result.png')
