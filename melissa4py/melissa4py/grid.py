import numpy as np

from functools import partial
from itertools import product, permutations


def cell2idx(cell, grid_dims):
    idx = 0
    for dim, c in enumerate(cell):
        idx += c * np.prod(grid_dims[:dim])
    return int(idx)


def sum_vectors(vectors, coeffs):
    result = np.zeros_like(vectors[0])
    for c, v in zip(coeffs, vectors):
        result += c * v
    return result


def build_pixel_connectivity_directions(dim=3, order=3):
    """
    Generate the directions we are allowed to move in order to get
    the neighbourhood determined by pixel connectivity in a grid of 
    dimention dim.

    Parameters
    ----------
    dim: integer
        Dimention of the grid
    order: integer
        Order of pixel connectivity.
    """
    directions = []
    basis_vectors = [np.eye(dim)[i] for i in range(dim)]
    for basis in permutations(basis_vectors, order):
        for coeffs in product([-1, 0, 1], repeat=order):
            direction = sum_vectors(basis, coeffs)
            directions.append(direction)
    return np.unique(np.array(directions), axis=0)


def get_pixel_connectivity_neighborhood(cell, directions=None, order=2,
                                        grid_dims=(10, 10, 10)):
    """
    Compute the neighborhood of a pixel. A set of direction vectors can be
    specified to save computation time if this operation is done repeatedly.

    Parameters
    ----------
    cell: np.array
        cell for which we are to compute the neighbours
    directions: np.array
        directions to compute the neighbourhood of a pixel
    order: integer
        pixel connectivity order
    grid_dims: tuple / iterable
        tuple or iterable with grid dimentions
    """
    if len(cell) != len(grid_dims):
        raise ValueError('The cell dimention should match the grid dimention | {} != {}'
                         .format(len(cell), len(grid_dims)))
    # 1. Compute dimentions of not given
    dim = len(cell.shape)
    if directions is None:
        directions = build_pixel_connectivity_directions(dim, order)
    # 2. Compute neighbours candidates
    candidates = cell + directions
    # 3. Filter canidates to get neighbours.
    neighbours = []
    for neighbour in candidates:
        if all(0 <= v < m for v, m in zip(neighbour, grid_dims)):
            neighbours.append(neighbour)
    return neighbours
            

def regular_grid_neighbourhood(grid_dims, order=3):
    """
    Compute the neighbourhood cells on a regular grid
    """
    dim = len(grid_dims)
    directions = build_pixel_connectivity_directions(dim=dim, order=order)
    neighbours = []
    for cell in product(*map(range, grid_dims)):
        cell = np.array(cell)
        neighborhood = get_pixel_connectivity_neighborhood(cell, directions,
                                                           grid_dims=grid_dims)
        neighborhood_idx = [[cell2idx(cell, grid_dims), cell2idx(n, grid_dims)]
                            for n in neighborhood]
        neighbours.extend(neighborhood_idx)
    return neighbours


def extend_neighbourhood(edge_list, number_of_parameters):
    extended_t = []
    for i, j in edge_list:
        for k1 in range(number_of_parameters):
            for k2 in range(number_of_parameters):
                d = [i * number_of_parameters + k1, j * number_of_parameters + k2]
                extended_t.append(d)
    return extended_t


def neighbors_read(filename):
    indices = []
    with open(filename, 'r') as f:
        for line in file:
            indices.append([int(i) for i in line.split()])
    return indices


def neighbors_write(indices, filename):
    with open(filename, 'w') as f:
        for i, j in indices:
            f.write('{} {}\n'.format(i, j))
