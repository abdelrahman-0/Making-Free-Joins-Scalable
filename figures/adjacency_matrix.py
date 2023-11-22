import matplotlib.pyplot as plt
import numpy as np


def generate_adj_mat(n):
    mat = np.zeros((n*n,n*n))
    for i in range(n*n):
        mat[i,(i*33)%(n*n)] = 1
        mat[i,(i*2)%(n*n)] = 1

    # make symmetric
    mat += mat.T
    mat[mat != 0] = 1
    # no self paths
    for i in range(n*n):
        mat[i,i] = 0

    return mat


def visualize_mat(n, mat):
    xs, ys = np.meshgrid(range(n), range(n))

    flattened_xs = xs.flatten()
    flattened_ys = ys.flatten()

    npts = mat.shape[0]
    for a in range(npts):
        for b in range(npts):
            if mat[a,b] == 1:
                plt.plot([flattened_xs[a], flattened_xs[b]], [flattened_ys[a], flattened_ys[b]], linewidth=0.5, color='grey')

    plt.scatter(xs,ys, s=5, color='red')
    plt.show()


if __name__ == '__main__':
    n = 3
    mat = generate_adj_mat(n)
    paths_2 = mat @ mat
    triangle = paths_2 @ mat
    print(np.sum(paths_2))
    print(triangle.diagonal())
    print(triangle.trace())
    visualize_mat(n, mat)
