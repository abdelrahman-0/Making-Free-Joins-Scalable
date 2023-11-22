import numpy as np
import matplotlib.pyplot as plt

def clique(n):
    return np.ones((n,n)) - np.diag(np.ones(n))


if __name__ == '__main__':

    ns = list(range(3,100))
    x = []
    y1 = []
    y2 = []
    for n in ns:
        c = clique(n)
        nedges = np.sum(c)
        x.append(nedges)
        y1.append(nedges*nedges)
        y2.append(np.sum(c@c))
    plt.plot(x,y1,label='squared')
    plt.plot(x,y2,label='paths of length 2')
    plt.xlabel('clique size')
    plt.legend()
    plt.show()