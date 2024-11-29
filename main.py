import bpy
import sys
import math
import bmesh
import numpy as np
import concurrent.futures
import NPerlinNoise as npn

interpreter_path = sys.executable
print("Interpreter path:", interpreter_path)

def clear_all():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)

    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)

def create_plane(location, size, subdivisions, height_array):
    # Calculate grid size
    grid_size = subdivisions + 2
    delta = size / (grid_size - 1)
    half_size = size / 2

    # Create a new mesh and object
    mesh = bpy.data.meshes.new("Plane")
    obj = bpy.data.objects.new("Plane", mesh)
    bpy.context.collection.objects.link(obj)

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
            faces.append([
                i,
                i + 1,
                i + grid_size + 1,
                i + grid_size,
            ])

    # Create mesh
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    for poly in mesh.polygons:
        poly.use_smooth = True

def get_noise_map(chunk_size, world_size, seed=None, frequency=8, waveLenght=128, range=(-.5, .5), octaves=8, persistence=0.5, lacunarity=2):
    size = chunk_size * world_size + 1

    # Initialize the noise object
    """
    seed:
        Specifies the seed for the random number generator.
        Using the same seed will produce the same noise pattern every time.
        If None, a random seed is used.
    frequency:
        Determines the frequency of the noise.
        Higher frequencies result in more rapid changes in the noise values over space, creating finer details.
    waveLength:
        Sets the wavelength of the noise, controlling the scale of the noise features.
        A larger wavelength produces smoother, larger patterns, while a smaller wavelength yields more detailed patterns.
    warp:
        Applies a warp transformation to the noise, adding complexity by distorting the noise pattern.
        If None, no warp is applied.
    _range:
        Defines the output range of the noise values.
        If specified, the noise values are scaled to fit within this range.
    octaves:
        The number of layers of noise to combine.
        Each octave adds additional detail by layering noise at increasing frequencies.
    persistence:
        Controls the amplitude of each successive octave.
        A lower value reduces the influence of higher octaves, resulting in smoother noise.
        A higher value increases the detail by emphasizing higher octaves.
    lacunarity:
        Determines the frequency multiplier between octaves.
        A higher lacunarity increases the frequency for each subsequent octave, adding finer details to the noise.
    """
    noise = npn.Noise(
        seed=seed,
        frequency=frequency,
        waveLength=waveLenght,
        warp=None,
        _range=range,
        octaves=octaves,
        persistence=persistence,
        lacunarity=lacunarity
    )

    # Define the coordinate ranges
    x_range = (0, size)
    y_range = (0, size)

    # Generate the noise map using perlinGenerator
    noise_map, coordsMesh = npn.perlinGenerator(noise, x_range, y_range)

    print(f"Noise map shape: {noise_map.shape}")
    return noise_map

def get_distance_map(chunk_size, world_size, origin=(0, 0)):
    size = chunk_size * world_size
    half_size = size // 2

    # Create coordinate grids
    x_coords = np.arange(-half_size, half_size + 1)
    y_coords = np.arange(-half_size, half_size + 1)
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

def create_chunk(location, size, subdivisions, map):
    # Calculate world location
    x_chunk, y_chunk = location[:2]
    world_x = x_chunk * size
    world_y = y_chunk * size
    world_location = (world_x, world_y, location[2])

    # Compute some values
    grid_size = subdivisions + 2  # Number of vertices per axis
    half_size = map.shape[0] // 2  # Assuming square image

    # Compute the position of this chunk in the modifier image
    x_start = int(world_x - (size / 2)) + half_size
    y_start = int(world_y - (size / 2)) + half_size
    x_end = x_start + grid_size
    y_end = y_start + grid_size

    # Extract the relevant section from the modifier image
    heights = map[y_start:y_end, x_start:x_end]

    create_plane(world_location, size, subdivisions, heights)

def create_terrain():
    # Clear existing objects
    clear_all()

    # Settings
    chunk_size = 64
    subdivisions = chunk_size - 1
    int_world_size = 15

    world_size = int_world_size * 2 + 1

    noise_map = get_noise_map(
        chunk_size,
        world_size,
        seed=0,
        frequency=128,
        waveLenght=8192,
        range=(0, 20),
        octaves=3,
        persistence=.5,
        lacunarity=2
    )
    height_modifier_map = get_distance_map(chunk_size, world_size, origin=(0, 0))

    heights = noise_map

    # Create terrain chunks
    for x in range(-int_world_size, int_world_size + 1):
        for y in range(-int_world_size, int_world_size + 1):
            if math.ceil(math.sqrt(x**2 + y**2) - .75) <= int_world_size:
                create_chunk(
                    location=(x, y, 0),
                    size=chunk_size,
                    subdivisions=subdivisions,
                    map=heights,
                )

    # Update the viewport
    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)

if __name__ == '__main__':
    create_terrain()