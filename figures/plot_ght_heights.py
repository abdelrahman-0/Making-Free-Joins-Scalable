import numpy as np
import plotly.express as px
import pandas as pd
import plotly.graph_objects as go
from plotly.colors import n_colors
import matplotlib
import plotly.io as pio
pio.kaleido.scope.mathjax = None # needed to remove tiny text at the bottom of figure
import matplotlib.pyplot as plt


def discrete_colorscale(bvals, colors):
    """
    bvals - list of values bounding intervals/ranges of interest
    colors - list of rgb or hex colorcodes for values in [bvals[k], bvals[k+1]],0<=k < len(bvals)-1
    returns the plotly  discrete colorscale
    """
    if len(bvals) != len(colors)+1:
        raise ValueError('len(boundary values) should be equal to  len(colors)+1')
    bvals = sorted(bvals)     
    nvals = [(v-bvals[0])/(bvals[-1]-bvals[0]) for v in bvals]  #normalized values
    
    dcolorscale = [] #discrete colorscale
    for k in range(len(colors)):
        dcolorscale.extend([[nvals[k], colors[k]], [nvals[k+1], colors[k]]])
    return dcolorscale    


def parse(path: str):
    with open(path, 'r') as f:
        lines = f.readlines()
    current_query = ''
    n_threads = -1
    data = []
    tup = []
    i = 0
    while i+1 < len(lines):
        l = lines[i].strip()
        l2 = lines[i+1].strip()
        if l.startswith('Query'):
            current_query = l[l.rindex(':')+2:]
            i += 1
        elif l.startswith('Using:'):
            n_threads = int(l.split(' ')[1])
            i += 1
        elif l.startswith('Num Tuples') and l2.startswith('Trie Height'):
            num_tuples = int(l[l.index(':')+2:].strip())
            ght_height = int(l2[l2.index(':')+2:].strip())
            tup = [current_query, n_threads, num_tuples, ght_height]
            data.append(tup)
            i += 2
        else:
            i += 1
    return data


def data_to_csv(data):
    df = pd.DataFrame(data=data, columns=['query', 'n_threads', 'number of tuples', 'GHT height'])
    return df


if __name__ == '__main__':
    # read data

    # log_path = '../logs/fjp_job_robin_true_default_14092023_185400.txt'
    log_path = '../logs/fjp_job_robin_true_smart_original_18092023_144800.txt'

    data = parse(log_path)
    df = data_to_csv(data)
    df = df[df['n_threads'] == 32]
    df = df.set_index('query')
    with open('order_smart_original.txt', 'r') as f:
        l = list(map(lambda x: x.strip()[:-1], f.readlines()))
    df = df.loc[l].reset_index()

    fig = px.scatter(df, x="query", y="number of tuples", color='GHT height',
                    color_continuous_scale=[(0.00, "black"),   (0.25, "black"),
                                            (0.25, "pink"),   (0.50, "pink"),
                                            (0.50, "blue"),   (0.75, "blue"),
                                            (0.75, "red"),   (1.00, "red")])
    fig.update_yaxes(type='log', range=[-0.5,8])
    
    fig.show()
    exit()

    max_depth = 4
    max_height = 3.7e6

    mat = np.asarray(df['atoms'].apply(lambda l: l + [0]*(width - len(l))).to_list(), dtype=int)
    max_val = 7
    cmap = plt.get_cmap('Blues', max_val + 1)
    # scale = []
    # for i in range(width+1):
    #     c = cmap(i/(width+1), bytes=True)[:3]
    #     color = 'rgb(' + str(c[0]) + ',' + str(c[1]) + ',' + str(c[2]) + ')'
    #     for j in [i, i+1]:
    #         scale += [(j/(width+1), color)]
    fig, ax = plt.subplots(figsize=(15, 2.5))
    im = ax.imshow(mat.T[::-1],cmap=cmap, vmin=-.5, vmax=max_val+.5, aspect='auto')
    from mpl_toolkits.axes_grid1 import make_axes_locatable
    divider = make_axes_locatable(ax)
    cax = divider.append_axes('right', size='2%', pad=0.05)
    fig.colorbar(im, cax=cax)
    ax.set_xticks(list(range(df.shape[0])), df['query'], fontsize=10, rotation=-90)
    ax.set_yticks(range(width), range(width)[::-1])
    ax.set_ylim(width-0.5, -0.5)
    cax.set_yticks(range(max_val+1))
    cax.set_ylabel('GHT height', fontsize=13)
    ax.set_xlabel('query', fontsize=13)
    ax.set_ylabel('number of tuples', fontsize=13)
    # plt.show()
    plt.tight_layout()
    plt.show()
    # fig.savefig('plots/fjp_ght_heights.pdf')