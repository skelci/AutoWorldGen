import bpy
import sys
import math
import time
import numpy as np
import concurrent.futures
import NPerlinNoise as npn

def clear_all():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)
    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)

def add_global_light():
    # Create a new light data block
    light_data = bpy.data.lights.new(name="GlobalLight", type='SUN')
    light_data.energy = 3  # Adjust the intensity as needed

    # Create a new object with the light data
    light_object = bpy.data.objects.new(name="GlobalLight", object_data=light_data)

    # Link the light object to the current collection
    bpy.context.collection.objects.link(light_object)

    # Position the light above the terrain
    light_object.location = (0, 0, 1024)

    # Rotate the light to cast shadows appropriately
    light_object.rotation_euler = (math.radians(50), 0, math.radians(30))

def prepare_plane_data(location, size, subdivisions, height_array, biome_influences):
    grid_size = subdivisions + 2
    delta = size / (grid_size - 1)
    half_size = size / 2

    # Define colors for each biome
    plains_color = (0.0, 0.9, 0.0)       # Green
    hills_color = (0.6, 0.3, 0.0)        # Brown
    mountains_color = (0.5, 0.5, 0.5)    # Gray

    verts = []
    colors = []
    for y in range(grid_size):
        for x in range(grid_size):
            co_x = -half_size + x * delta + location[0]
            co_y = -half_size + y * delta + location[1]
            co_z = height_array[y, x]
            verts.append((co_x, co_y, co_z))

            # Get biome influences
            plains_influence = biome_influences['plains'][y, x]
            hills_influence = biome_influences['hills'][y, x]
            mountains_influence = biome_influences['mountains'][y, x]

            # Calculate vertex color based on biome influences
            r = (plains_color[0] * plains_influence +
                 hills_color[0] * hills_influence +
                 mountains_color[0] * mountains_influence)
            g = (plains_color[1] * plains_influence +
                 hills_color[1] * hills_influence +
                 mountains_color[1] * mountains_influence)
            b = (plains_color[2] * plains_influence +
                 hills_color[2] * hills_influence +
                 mountains_color[2] * mountains_influence)

            color = (r, g, b, 1.0)  # RGBA
            colors.append(color)

    faces = []
    for y in range(grid_size - 1):
        for x in range(grid_size - 1):
            i = x + y * grid_size
            faces.append([i, i + 1, i + grid_size + 1, i + grid_size])

    return verts, faces, colors

def get_chunk_biome_influences(x_chunk, y_chunk, size, subdivisions, biome_maps):
    grid_size = subdivisions + 2
    half_size = biome_maps['plains'].shape[0] // 2

    world_x = x_chunk * size
    world_y = y_chunk * size

    x_start = int(world_x - (size / 2)) + half_size
    y_start = int(world_y - (size / 2)) + half_size
    x_end = x_start + grid_size
    y_end = y_start + grid_size

    chunk_biomes = {}
    for biome_name, biome_map in biome_maps.items():
        chunk_biomes[biome_name] = biome_map[y_start:y_end, x_start:x_end]

    return chunk_biomes

def create_mesh_object(name, verts, faces, colors):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    # Set smooth shading
    for poly in mesh.polygons:
        poly.use_smooth = True

    # Create object
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    # Add vertex color layer
    color_layer = mesh.vertex_colors.new(name="BiomeColors")
    color_data = color_layer.data

    # Assign colors to vertices
    for poly in mesh.polygons:
        for idx in poly.loop_indices:
            loop_vert_index = mesh.loops[idx].vertex_index
            color = colors[loop_vert_index]
            color_data[idx].color = color

    # Assign material 'BiomeBase' to the object
    material_name = 'BiomeBase'
    if material_name in bpy.data.materials:
        material = bpy.data.materials[material_name]
    else:
        # Create a new material
        material = bpy.data.materials.new(name=material_name)
        material.use_nodes = True

        # Clear default nodes
        material.node_tree.nodes.clear()

        # Create nodes
        nodes = material.node_tree.nodes
        links = material.node_tree.links

        # Add nodes
        output_node = nodes.new(type='ShaderNodeOutputMaterial')
        bsdf_node = nodes.new(type='ShaderNodeBsdfPrincipled')
        attribute_node = nodes.new(type='ShaderNodeAttribute')
        attribute_node.attribute_name = "BiomeColors"

        # Link nodes
        links.new(attribute_node.outputs['Color'], bsdf_node.inputs['Base Color'])
        links.new(bsdf_node.outputs['BSDF'], output_node.inputs['Surface'])

    # Assign the material to the object
    if obj.data.materials:
        obj.data.materials[0] = material
    else:
        obj.data.materials.append(material)

def create_terrain():
    t = [time.time()]

    t.append(time.time())

    # Settings
    chunk_size = 64
    subdivisions = chunk_size - 1
    int_world_size = 31
    world_size = int_world_size * 2 + 1

    # Generate noise maps
    with concurrent.futures.ThreadPoolExecutor() as executor:
        plains_future = executor.submit(
            get_noise_map,
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
        hills_future = executor.submit(
            get_noise_map,
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
        mountains_future = executor.submit(
            get_noise_map,
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

        plains = plains_future.result()
        hills = hills_future.result()
        mountains = mountains_future.result()

    t.append(time.time())

    # Generate biome influence maps
    plains_biome = get_distance_map(chunk_size, world_size, origin=(1, 2))
    plains_biome = 1 - sigmoid(plains_biome, a=3, s=16 * chunk_size, k=1 / chunk_size)

    hills_biome = get_distance_map(chunk_size, world_size, origin=(-1, 1))
    hills_biome = 1 - sigmoid(hills_biome, a=1.5, s=32 * chunk_size, k=1 / chunk_size)

    mountains_biome = 1 - hills_biome
    hills_biome = hills_biome - plains_biome

    biome_maps = {
        'plains': plains_biome,
        'hills': hills_biome,
        'mountains': mountains_biome
    }

    # Apply biome influences to noise maps
    plains *= plains_biome
    hills *= hills_biome
    mountains *= mountains_biome

    noise_map = plains + hills + mountains

    t.append(time.time())

    # Prepare chunk coordinates
    chunk_coords = [
        (x, y)
        for x in range(-int_world_size, int_world_size + 1)
        for y in range(-int_world_size, int_world_size + 1)
        if math.ceil(math.sqrt(x ** 2 + y ** 2) - 0.75) <= int_world_size
    ]
    t.append(time.time())

    # Function to prepare data for a chunk
    def prepare_chunk_data(coords):
        x, y = coords
        location = (x * chunk_size, y * chunk_size, 0)
        heights_chunk = get_chunk_heights(x, y, chunk_size, subdivisions, noise_map)
        biome_influences = get_chunk_biome_influences(x, y, chunk_size, subdivisions, biome_maps)
        verts, faces, colors = prepare_plane_data(location, chunk_size, subdivisions, heights_chunk, biome_influences)
        name = f"Plane_{x}_{y}"
        return (name, verts, faces, colors)

    # Prepare data in parallel
    with concurrent.futures.ThreadPoolExecutor() as executor:
        mesh_data_list = list(executor.map(prepare_chunk_data, chunk_coords))

    t.append(time.time())

    # Create meshes
    for name, verts, faces, colors in mesh_data_list:
        create_mesh_object(name, verts, faces, colors)

    t.append(time.time())

    # Update the viewport
    bpy.context.view_layer.update()
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)
    t.append(time.time())

    for i in range(len(t) - 1):
        print(f"Step {i}: {t[i + 1] - t[i]:.4f} seconds")

def sigmoid(x, a=2, s=0, k=1):
    return 1 / (1 + a ** (-(k * x - k * s)))

def get_distance_map(chunk_size, world_size, origin=(0, 0)):
    size = chunk_size * world_size + ((world_size - 1) // 2) % 2 + 1
    half_size = size // 2

    x_coords = np.arange(-half_size, size - half_size)
    y_coords = np.arange(-half_size, size - half_size)
    x_indices, y_indices = np.meshgrid(x_coords, y_coords)

    origin_x, origin_y = origin

    x_diff = np.abs(x_indices - origin_x * chunk_size)
    y_diff = np.abs(y_indices - origin_y * chunk_size)

    values = np.sqrt(x_diff ** 2 + y_diff ** 2) * np.sqrt(2)

    modifier_map = values.astype(np.float32)
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
    return noise_map

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

if __name__ == '__main__':
    clear_all()
    add_global_light()
    create_terrain()
