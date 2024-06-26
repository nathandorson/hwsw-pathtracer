from enum import IntEnum
import json
import math
import matplotlib.pyplot as plt
import numpy as np
import random
import struct
from collections import deque
from time import time
from typing import List, Optional

NUM_PLL = 16
RAY_FIELDS = 8
RAYHIT_FIELDS = 4
FIELD_WIDTH = 4


def rot_vec_z(vec, c, s) -> np.ndarray:
    """
    Rotate a 3D vector around the Z axis.

    Parameters:
        vec: The vector to rotate.
        c: The cosine of the angle to rotate by.
        s: The sine of the angle to rotate by.

    Returns:
        np.ndarray: The rotated vector.
    """
    nx = vec[0] * c + vec[1] * s
    ny = vec[0] * s - vec[1] * c
    return np.array([nx, ny, vec[2]])

def rot_vec_y(vec, c, s) -> np.ndarray:
    """
    Rotate a 3D vector around the Y axis.

    Parameters:
        vec: The vector to rotate.
        c: The cosine of the angle to rotate by.
        s: The sine of the angle to rotate by.

    Returns:
        np.ndarray: The rotated vector.
    """
    nx = vec[0] * c + vec[2] * s;
    nz = vec[2] * c - vec[0] * s;
    return np.array([nx, vec[1], nz])

def color_mult(a, b) -> np.ndarray:
    """
    Multiply two colors together.

    Parameters:
        a: The first color to multiply.
        b: The second color to multiply.

    Returns:
        np.ndarray: The multiplied color.
    """
    return a * (b / 255)

def normalize(vec) -> np.ndarray:
    """
    Normalize a 3D vector.

    Parameters:
        vec: The vector to normalize.

    Returns:
        np.ndarray: The normalized vector.
    """
    norm = np.linalg.norm(vec)
    if norm == 0:
        return vec
    return vec / norm

# TODO: Not sure how well hardware will support random or trig ops, so may share a memory chunk
#       with a bunch of random vectors / random points on the unit sphere.
def random_vector() -> np.ndarray:
    """
    Generate a random 3D vector on the unit sphere.

    Returns:
        np.ndarray: A random 3D vector on the unit sphere.
    """
    u = random.random()
    v = random.random()
    theta = u * 2 * math.pi
    phi = math.acos(2 * v - 1)
    sin_phi = math.sin(phi)
    return np.array([sin_phi * math.cos(theta), sin_phi * math.sin(theta), math.cos(phi)])

def random_hemisphere_vector(normal) -> np.ndarray:
    """
    Generate a random 3D vector in the hemisphere defined by the given normal.

    Parameters:
        normal: The normal defining the hemisphere.

    Returns:
        np.ndarray: A random 3D vector in the hemisphere defined by the given normal.
    """
    v = random_vector()
    if np.dot(v, normal) < 0:
        # Random vector not in the same hemisphere as normal - flip it.
        return -1 * v
    return v

class Ray():
    """Represents a ray in 3D space."""

    def __init__(self, pos, dir, bounces=0, r=0, c=0) -> None:
        """
        Initialize a new Ray object.

        Parameters:
            pos: The starting position of the ray.
            dir: The direction of travel of the ray.
            bounces: The number of bounces the ray has made.
            r: The row of the pixel.
            c: The column of the pixel.
        """
        self.pos: np.ndarray = pos
        self.dir: np.ndarray = dir
        self.bounces = bounces
        self.r = r
        self.c = c

class Camera():
    """Represents a camera in 3D space."""

    def __init__(self, pos, pitch, yaw, rows, cols, fov):
        """
        Initialize a new Camera object.

        Parameters:
            pos: The position of the camera.
            pitch: The pitch of the camera.
            yaw: The yaw of the camera.
            rows: The number of rows in the camera's view.
            cols: The number of columns in the camera's view.
            fov: The field of view of the camera.
        """
        self.pos: np.ndarray = pos
        self.pitch = pitch
        self.yaw = yaw
        self.fov_x: float = fov
        self.fov_y: float = fov * (rows / cols)
        self.rows: int = rows
        self.cols: int = cols
        self.half_view_width: float = math.tan(self.fov_x / 2.0)
        self.half_view_height: float = math.tan(self.fov_y / 2.0)

        self.top_left: np.ndarray
        self.top_right: np.ndarray
        self.bottom_left: np.ndarray

        self.horizontal_delta: np.ndarray
        self.vert_delta: np.ndarray

        self.precompute()

    def precompute(self):
        """
        Precompute the camera's view vectors.
        """
        pc = math.cos(self.pitch)
        ps = math.sin(self.pitch)
        yc = math.cos(self.yaw)
        ys = math.sin(self.yaw)
        self.top_left = rot_vec_z(rot_vec_y(np.array([1, self.half_view_width, -1 * self.half_view_height]), pc, ps), yc, ys)
        self.bottom_left = rot_vec_z(rot_vec_y(np.array([1, self.half_view_width, self.half_view_height]), pc, ps), yc, ys)
        self.top_right = rot_vec_z(rot_vec_y(np.array([1, -1 * self.half_view_width, -1 * self.half_view_height]), pc, ps), yc, ys)
        self.horizontal_delta = (self.top_right - self.top_left) / self.cols
        self.vert_delta = (self.bottom_left - self.top_left) / self.rows

    def get_ray_dir(self, x, y):
        """
        Get the ray originating at the camera location and pointed towards the given pixel position.

        Parameters:
            x: The x position of the pixel.
            y: The y position of the pixel.

        Returns:
            np.ndarray: The dir of the ray originating at the camera location and pointed towards the given pixel position.
        """
        dir: np.ndarray = self.top_left + (self.horizontal_delta * x) + (self.vert_delta * y)
        norm = np.linalg.norm(dir)
        if norm != 0:
            dir = dir / norm
        return dir

    def get_random_ray(self, r, c) -> Ray:
        """
        Get a ray originating at the camera location and pointed towards a random position within the given pixel.

        Parameters:
            r: The row of the pixel.
            c: The column of the pixel.

        Returns:
            Ray: A ray originating at the camera location and pointed towards a random position within the given pixel.
        """
        return Ray(self.pos, self.get_ray_dir(c + random.random(), r + random.random()), r=r, c=c)

class Intersection():
    """Represents an intersection between a ray and a shape."""

    def __init__(self, pt, shape, dist):
        """
        Initialize a new Intersection object.

        Parameters:
            pt: The point of intersection.
            shape: The shape that was intersected.
            dist: The distance from the ray's starting position to the point of intersection.
        """
        self.pt = pt
        self.shape = shape
        self.dist = dist

class ShapeType(IntEnum):
    """Represents the type of a shape."""
    PLANE = 0
    SPHERE = 1
    TRIANGLE = 2

def shape_type(shape_str: str) -> ShapeType:
    """
    Convert a string to a ShapeType.

    Parameters:
        shape_str: The string to convert.

    Returns:
        ShapeType: The converted ShapeType.
    """
    if shape_str == "sphere":
        return ShapeType.SPHERE
    if shape_str == "plane":
        return ShapeType.PLANE
    if shape_str == "triangle":
        return ShapeType.TRIANGLE
    raise Exception("Invalid shape type.")

class Shape:
    """Represents a shape in 3D space."""

    def __init__(self, s_type, color, specularity, emittance, coordinates):
        """
        Initialize a new Shape object.

        Parameters:
            s_type: The type of the shape.
            color: The color of the shape.
            specularity: The specularity of the shape.
            emittance: The emittance of the shape.
            coordinates: The coordinates of the shape.
        """
        self.shape_type = shape_type(s_type)
        self.color = color
        self.specularity = specularity
        self.emittance = emittance
        # Note that all three coordinates only used for triangles.
        # Else the first coord defines center and second defines normal.
        self.coordinates = coordinates

    def tobytes(self) -> bytes:
        # todo ensure functioning
        return struct.pack('fffffffffc0i', *self.coordinates[0], *self.coordinates[1], *self.coordinates[2], bytes([self.shape_type]))

    def intersection_with(self, ray) -> Optional[Intersection]:
        """
        Get the intersection between the shape and the given ray.

        Parameters:
            ray: The ray to check for intersection.

        Returns:
            Optional[Intersection]: The intersection between the shape and the given ray, if it exists.
        """
        if self.shape_type == ShapeType.PLANE:
            dir_dot_norm = np.dot(ray.dir, self.coordinates[1])
            if abs(dir_dot_norm) < 0.000001:
                return None
            t = np.dot(self.coordinates[0] - ray.pos, self.coordinates[1]) / dir_dot_norm
            if t < 0:
                return None
            traversed_ray = t * ray.dir
            intersect_pt = ray.pos + traversed_ray
            return Intersection(intersect_pt, self, np.linalg.norm(traversed_ray))

        elif self.shape_type == ShapeType.SPHERE:
            a = np.dot(ray.dir, ray.dir)
            b = 2 * np.dot(ray.dir, ray.pos - self.coordinates[0])
            c = np.dot(ray.pos - self.coordinates[0], ray.pos - self.coordinates[0]) - self.coordinates[1][0] ** 2 # XXX: coordinates[1][0] as radius
            discrim = (b ** 2) - (4 * a * c)
            if discrim < 0:
                return None
            t1 = (-1 * b + math.sqrt(discrim)) / (2 * a)
            t2 = (-1 * b - math.sqrt(discrim)) / (2 * a)
            if t1 < 0 and t2 < 0:
                return None
            t = min(t1, t2)
            traversed_ray = t * ray.dir
            intersect_pt = ray.pos + traversed_ray
            return Intersection(intersect_pt, self, np.linalg.norm(traversed_ray))

        elif self.shape_type == ShapeType.TRIANGLE:
            edge1 = self.coordinates[1] - self.coordinates[0]
            edge2 = self.coordinates[2] - self.coordinates[0]
            ray_cross_edge2 = np.cross(ray.dir, edge2)
            det = np.dot(edge1, ray_cross_edge2)
            if abs(det) < 0.000001:
                return None
            inv_det = 1.0 / det
            s = ray.pos - self.coordinates[0]
            u = inv_det * np.dot(s, ray_cross_edge2)
            if (u < 0) or (u > 1):
                return None
            s_cross_edge1 = np.cross(s, edge1)
            v = inv_det * np.dot(ray.dir, s_cross_edge1)
            if (v < 0) or (u + v > 1):
                return None
            t = inv_det * np.dot(edge2, s_cross_edge1)
            if (t < 0.000001):
                return None
            traversed_ray =  t * ray.dir
            intersect_pt = ray.pos + traversed_ray
            return Intersection(intersect_pt, self, np.linalg.norm(traversed_ray))
        
        else:
            return None

    def normal(self, point) -> np.ndarray:
        """
        Get the normal of the shape at the given point.

        Parameters:
            point: The point at which to get the normal.

        Returns:
            np.ndarray: The normal of the shape at the given point.
        """
        if self.shape_type == ShapeType.PLANE:
            return self.coordinates[1]
        if self.shape_type == ShapeType.SPHERE:
            return normalize(point - self.coordinates[0])
        if self.shape_type == ShapeType.TRIANGLE:
            return np.cross(self.coordinates[1] - self.coordinates[0], self.coordinates[2] - self.coordinates[0])
        raise Exception()

def load_scene_from_json(json_blob):
    """
    Load a scene from a JSON blob.

    Parameters:
        json_blob: The JSON blob to load.

    Returns:
        List[Shape]: List of shapes representing the loaded scene.
    """
    shapes = []
    data = json.loads(json_blob)
    for obj in data:
        shape_type = obj['shape_type']
        color = np.array(obj['color'])
        specularity = obj['specularity']
        emittance = obj['emittance']
        coordinates = [np.array(coord) for coord in obj['coordinates']]
        shape = Shape(shape_type, color, specularity, emittance, coordinates)
        shapes.append(shape)
    return shapes

class Pathtracer():
    """Represents a path tracer."""

    def __init__(self, rows=360, cols=480, camera = None) -> None:
        """
        Initialize a new Pathtracer object.

        Parameters:
            camera: The camera to use for rendering.
        """
        self.rays_per_pixel = 4
        self.depth = 4
        
        self.rows = rows
        self.cols = cols
        self.fov = 90
        self.camera: Camera = Camera([0, 0, 0], 0, 0, self.rows, self.cols, self.fov) # TODO pass this in from json, as it helps describe the scene to trace.

        self.scene: List[Shape] = []

        # Can be used for accumulating / calculating pixel colors.
        self.pixels = np.zeros((self.rows, self.cols, 3))

        # Must contain the final pixel colors.
        self.final_pixels = np.zeros((self.rows, self.cols, 3))

        self.iters = 0
        self.done = 0

    def load_from_file(self, json_file) -> None:
        """
        Load a scene for this pathtracer from a JSON file.

        Parameters:
            json_file: The JSON file to load.
        """
        with open(json_file, 'r') as scene_file:
            json_blob = scene_file.read()
            self.scene = load_scene_from_json(json_blob)

    def cast_ray(self, ray) -> Optional[Intersection]:
        """
        Cast a ray into the scene and find the closest intersection.

        Parameters:
            ray: The ray to cast.

        Returns:
            Optional[Intersection]: The closest intersection, if it exists.
        """
        intersection: Optional[Intersection] = None
        closest_dist = 99999
        for shape in self.scene:
            if isect := shape.intersection_with(ray):
                if isect.dist < closest_dist and isect.dist > 0.0001:
                    closest_dist = isect.dist
                    intersection = isect
        return intersection

    def ray_color(self, ray: Ray):
        """
        Get the color of a given ray should it be bounced around the scene
        subject to the pathtracing algorithm.

        Parameters:
            ray: The ray to cast.

        Returns:
            np.ndarray: The color of the simulated ray, as an np.ndarray.
        """
        traced_ray = ray
        traced_color = np.array([255, 255, 255])

        for bounce_num in range(self.depth):
            intersection: Optional[Intersection] = self.cast_ray(traced_ray)
            self.iters += 1

            if intersection is None:
                return np.array([0, 0, 0])

            shape = intersection.shape 

            if (emittance := shape.emittance) > 0:
                traced_color = color_mult(traced_color, emittance * shape.color)
                break
            elif bounce_num == self.depth - 1:
                # Last bounce needs to hit a light, else the ray will be dark.
                return np.array([0, 0, 0])

            normal = shape.normal(intersection.pt)

            # If normal vector and ray point in same hemisphere, flip the normal.
            if np.dot(normal, traced_ray.dir) > 0.0:
                normal = normal * -1

            # XXX: Ambient light not used at the moment.
            ambient_color = shape.color * shape.emittance
            diffuse_dir = random_hemisphere_vector(normal)
            traced_color = color_mult(traced_color, shape.color) * np.dot(normal, diffuse_dir) # XXX: * 2

            # TODO: Maybe allow for specular bouncing eventually.
            #       Requires splitting rays, or randomly selecting if to do diffuse or spec bouncing.

            traced_ray = Ray(intersection.pt, diffuse_dir)

        return traced_color

    def init_hardware(self):
        global allocate
        from pynq import Overlay
        from pynq import allocate

        self.ol = Overlay('/home/xilinx/pynq/overlays/raycast/raycast.bit')

        self.raycast_ip = self.ol.raycast_0
        self.dma = self.ol.axi_dma
        self.dma_send = self.dma.sendchannel
        self.dma_recv = self.dma.recvchannel

        self.scene_bram = self.ol.axi_bram_ctrl_0

    def send_scene_to_hardware(self):
        for i, shape in enumerate(self.scene):
            # 64 byte offset from one shape to the next
            self.scene_bram.write(64 * i, shape.tobytes())
        print("Scene synced to hardware.")

    def hardware_send_recv(self, rays) -> List[Optional[Intersection]]:
        for i, ray in enumerate(rays):
            # TODO if using custom fixed point, convert first
            struct.pack_into('ffffff', self.input_buffer, i * RAY_FIELDS * FIELD_WIDTH, *ray.pos, *ray.dir)

        START_CONTROL_REG = 0x0
        self.raycast_ip.write(START_CONTROL_REG, 0x1)
        self.dma_send.transfer(self.input_buffer)
        self.dma_recv.transfer(self.output_buffer)

        intersections = []

        rayhit_iter = struct.iter_unpack('fffi', self.output_buffer)

        for rayhit in rayhit_iter:
            (x, y, z, scene_idx) = rayhit
            if scene_idx == 0: # Scene hits from hardware returned one indexed so that 0 repr. no hit.
                intersections.append(None)
                continue
            intersections.append(Intersection(np.array([x,y,z]), self.scene[scene_idx - 1], 0))

        return intersections

    def software_send_recv(self, rays) -> List[Optional[Intersection]]:
        intersections: List[Optional[Intersection]] = []
        for ray in rays:
            if ray:
                intersections.append(self.cast_ray(ray))

        return intersections

    def trace_ray_group(self, ray_queue, num_pll, send_recv_fn):
        rays_to_run = min(len(ray_queue), num_pll)
        traced_rays = [ray_queue.popleft() for _ in range(rays_to_run)]

        self.iters += rays_to_run

        intersections = send_recv_fn(traced_rays)

        for i, ray in enumerate(traced_rays):
            intersection = intersections[i]

            if intersection is None:
                self.pixels[ray.r][ray.c] = np.array([0, 0, 0])
                self.done += 1
                continue

            shape = intersection.shape 

            if (emittance := shape.emittance) > 0:
                self.pixels[ray.r][ray.c] = color_mult(self.pixels[ray.r][ray.c], emittance * shape.color)
                self.done += 1
                continue

            if ray.bounces == self.depth - 1:
                # Last bounce needs to hit a light, else the ray will be dark.
                self.pixels[ray.r][ray.c] = np.array([0, 0, 0])
                self.done += 1
                continue

            normal = shape.normal(intersection.pt)

            # If normal vector and ray point in same hemisphere, flip the normal.
            if np.dot(normal, traced_rays[i].dir) > 0.0:
                normal = normal * -1

            diffuse_dir = random_hemisphere_vector(normal)
            self.pixels[ray.r][ray.c] = color_mult(self.pixels[ray.r][ray.c], shape.color) * np.dot(normal, diffuse_dir) # XXX: * 2

            ray_queue.append(Ray(intersection.pt, diffuse_dir, ray.bounces + 1, ray.r, ray.c))

    def render_scene_grouped(self, send_recv_fn):
        """
        Render the scene using the pathtracing algorithm on hardware.
        """
        self.iters = 0
        self.done = 0
        print(
            f"Running the pathtracer with a bounce depth of {self.depth} "
            f"and {self.rays_per_pixel} rays per pixel."
        )

        self.final_pixels = np.zeros((self.rows, self.cols, 3))

        for _ in range(self.rays_per_pixel):
            rays = deque()
            self.pixels = 255 * np.ones((self.rows, self.cols, 3))

            print("\rGenerating rays to trace from the camera...", end='')

            # Set up the initial rays.
            for r in range(self.rows):
                for c in range(self.cols):
                    rays.append(self.camera.get_random_ray(r, c))

            t0 = time()

            # Fire and requeue rays until we are done.
            while len(rays) > 0:
                self.trace_ray_group(rays, NUM_PLL, send_recv_fn)
                print(
                    f"\rTraced {self.iters} rays so far. {self.done // self.rays_per_pixel} / {self.rows * self.cols} pixels done. "
                    f"{self.iters / (time() - t0):.0f} rays per second.",
                    end=''
                )

            self.final_pixels += self.pixels

        self.final_pixels = self.final_pixels / self.rays_per_pixel

        print()
        print("Done!", " " * 64)

    def render_scene_in_hardware(self):
        self.input_buffer = allocate(shape=(NUM_PLL * RAY_FIELDS * FIELD_WIDTH,), dtype=np.byte)
        self.output_buffer = allocate(shape=(NUM_PLL * RAYHIT_FIELDS * FIELD_WIDTH,), dtype=np.byte)

        self.send_scene_to_hardware()

        return self.render_scene_grouped(self.hardware_send_recv)

    def render_scene_in_software(self):
        return self.render_scene_grouped(self.software_send_recv)

    def render_scene(self):
        """
        Render the scene using the pathtracing algorithm.
        Logs progress to the console.
        Results in the pixels attribute being filled with the rendered image.
        """
        self.iters = 0
        self.done = 0
        print(
            f"Running the pathtracer with a bounce depth of {self.depth} "
            f"and {self.rays_per_pixel} rays per pixel."
        )
        for r in range(self.rows):
            for c in range(self.cols):
                pixel_color = np.zeros((3))
                for _ in range(self.rays_per_pixel):
                    ray = self.camera.get_random_ray(r, c)
                    color = self.ray_color(ray)
                    pixel_color += color
                pixel_color = pixel_color / self.rays_per_pixel
                self.pixels[r][c] = pixel_color
            print(f"{(100 * (r + 1) / self.rows):.2f}% done", end='\r')
        self.final_pixels = self.pixels.copy()
        print("Done!", " " * 20)

    def usable_pixel_array(self):
        """
        Get the pixel array in a format that can be saved to a file or displayed.
        """
        return self.final_pixels.clip(0, 255).astype('uint8')

    def save_rendered_scene(self, fname):
        """
        Save the rendered scene to a file.

        Parameters:
            fname: The file to save the rendered scene to.
        """
        plt.imsave(fname, self.usable_pixel_array())
        print(f"Saved rendered scene to {fname}")

