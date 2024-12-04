import bpy
import sys
import math
import time
import numpy as np
import concurrent.futures
import NPerlinNoise as npn

def clear_all():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)

    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)

def prepare_plane_data(location, size, subdivisions, height_array):
    # Calculate grid size and parameters
    grid_size = subdivisions + 2
    delta = size / (grid_size - 1)
    half_size = size / 2

    # Generate vertices
    verts = []
    for y in range(grid_size):
        for x in range(grid_size):
            co_x = -half_size + x * delta + location[0]
            co_y = -half_size + y * delta + location[1]
            co_z = height_array[y, x]
            verts.append((co_x, co_y, co_z))

    # Generate faces
    faces = []
    for y in range(grid_size - 1):
        for x in range(grid_size - 1):
            i = x + y * grid_size
            faces.append([i, i + 1, i + grid_size + 1, i + grid_size])

    return verts, faces

def get_chunk_heights(x_chunk, y_chunk, size, subdivisions, map):
    grid_size = subdivisions + 2
    half_size = map.shape[0] // 2

    world_x = x_chunk * size
    world_y = y_chunk * size

    x_start = int(world_x - (size / 2)) + half_size
    y_start = int(world_y - (size / 2)) + half_size
    x_end = x_start + grid_size
    y_end = y_start + grid_size

    return map[y_start:y_end, x_start:x_end]

def create_mesh_object(name, verts, faces):
    # Create mesh and object
    mesh = bpy.data.meshes.new(name)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    # Create mesh from data
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    # Set smooth shading
    for poly in mesh.polygons:
        poly.use_smooth = True

def get_distance_map(chunk_size, world_size, origin=(0, 0)):
    size = chunk_size * world_size + 1
    half_size = size // 2

    # Create coordinate grids
    x_coords = np.arange(-half_size, size - half_size + 1)
    y_coords = np.arange(-half_size, size - half_size + 1)
    x_indices, y_indices = np.meshgrid(x_coords, y_coords)

    origin_x, origin_y = origin

    # Compute the value at each coordinate relative to the origin
    x_diff = np.abs(x_indices - origin_x * chunk_size)
    y_diff = np.abs(y_indices - origin_y * chunk_size)

    # Calculate the value as per the formula
    values = np.sqrt(x_diff**2 + y_diff**2) * 2**.5

    modifier_map = values.astype(np.float32)
    print(f"Distance map shape: {modifier_map.shape}")
    return modifier_map

def get_noise_map(chunk_size, world_size, seed=None, frequency=8, waveLength=128, range=(-0.5, 0.5), octaves=8, persistence=0.5, lacunarity=2):
    size = chunk_size * world_size + 1

    noise = npn.Noise(
        seed=seed,
        frequency=frequency,
        waveLength=waveLength,
        warp=None,
        _range=range,
        octaves=octaves,
        persistence=persistence,
        lacunarity=lacunarity
    )

    x_range = (0, size)
    y_range = (0, size)

    noise_map, _ = npn.perlinGenerator(noise, x_range, y_range)
    print(f"Noise map shape: {noise_map.shape}")
    return noise_map

def create_terrain():
    t = [time.time()] #* 0
    # Clear existing objects
    clear_all()
    t.append(time.time()) #* 1

    # Settings
    chunk_size = 64
    subdivisions = chunk_size - 1
    int_world_size = 31
    world_size = int_world_size * 2 + 1

    # Generate noise map
    plains = get_noise_map(
        chunk_size,
        world_size,
        seed=None,
        frequency=48,
        waveLength=8192,
        range=(-16, 16),
        octaves=5,
        persistence=0.5,
        lacunarity=2
    )
    hills = get_noise_map(
        chunk_size,
        world_size,
        seed=None,
        frequency=24,
        waveLength=8192,
        range=(-8, 56),
        octaves=5,
        persistence=0.5,
        lacunarity=2
    )
    mountains = get_noise_map(
        chunk_size,
        world_size,
        seed=None,
        frequency=20,
        waveLength=8192,
        range=(0, 384),
        octaves=6,
        persistence=0.5,
        lacunarity=2
    )
    t.append(time.time()) #* 2

    def sigmoid(x, a=2, s=0, b=1):
        return 1 / (1 + a**(-(b*x - b*s)))

    plains_biome = get_distance_map(chunk_size, world_size, origin=(1, 2))
    plains_biome = 1 - sigmoid(plains_biome, a=3, s=16*world_size, b=1/world_size)

    hills_biome = get_distance_map(chunk_size, world_size, origin=(-1, 1))
    hills_biome = 1 - sigmoid(hills_biome, a=1.5, s=32*world_size, b=1/world_size)

    mountains_biome = 1 - hills_biome
    hills_biome = hills_biome - plains_biome

    plains = plains * plains_biome
    hills = hills * hills_biome
    mountains = mountains * mountains_biome

    noise_map = plains + hills + mountains

    t.append(time.time()) #* 3

    # Prepare chunk coordinates
    chunk_coords = [
        (x, y)
        for x in range(-int_world_size, int_world_size + 1)
        for y in range(-int_world_size, int_world_size + 1)
        if math.ceil(math.sqrt(x**2 + y**2) - 0.75) <= int_world_size
    ]
    t.append(time.time()) #* 4

    # Function to prepare data for a chunk
    def prepare_chunk_data(coords):
        x, y = coords
        location = (x * chunk_size, y * chunk_size, 0)
        heights_chunk = get_chunk_heights(x, y, chunk_size, subdivisions, noise_map)
        verts, faces = prepare_plane_data(location, chunk_size, subdivisions, heights_chunk)
        name = f"Plane_{x}_{y}"
        return (name, verts, faces)

    # Prepare data in parallel
    with concurrent.futures.ThreadPoolExecutor() as executor:
        mesh_data_list = list(executor.map(prepare_chunk_data, chunk_coords))
    t.append(time.time()) #* 5

    # Create meshes in the main thread
    for name, verts, faces in mesh_data_list:
        create_mesh_object(name, verts, faces)
    t.append(time.time()) #* 6

    # Update the viewport
    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)
    t.append(time.time()) #* 7

    for i in range(len(t) - 1):
        print(f"Step {i}: {t[i + 1] - t[i]:.4f} seconds")

if __name__ == '__main__':
    create_terrain()