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
    while i < len(lines):
        l = lines[i].strip()
        if l.startswith('Query'):
            if tup:
                data += [tup]
            current_query = l[l.rindex(':')+2:]
            i += 1
        elif l.startswith('Using:'):
            n_threads = int(l.split(' ')[1])
            i += 1
        elif l.startswith('Num ') and 'Tuples' not in l:
            l2 = lines[i+1].strip()
            l3 = lines[i+2].strip()
            num_nodes = int(l[l.index(':')+1:].strip())
            num_atoms = list(map(lambda x: int(x.strip()), l2[l2.index(':')+1:].split(' ')))
            num_attributes = list(map(lambda x: int(x.strip()), l3[l3.index(':')+1:].split(' ')))
            tup = [current_query, n_threads, num_nodes, num_atoms, num_attributes]
            i += 3

            while i < len(lines) and not lines[i].strip().startswith('Query'):
                i += 1
            if i < len(lines) and lines[i].strip().startswith('Query'):
                i -= 1
        else:
            i += 1
    if tup:
        data += [tup]
    return data

def data_to_csv(data):
    raw_df = []
    for d in data:
        if d[1] != 12:
            continue
        q = d[0]
        raw_df += [[q, d[3], d[4]]]
        # for (natom, nattr) in zip(d[3], d[4]):
        #     raw_df += [[q, natom, nattr]]
    df = pd.DataFrame(data=raw_df, columns=['query', 'atoms', 'attributes'])
    return df


if __name__ == '__main__':
    # read data
    # log_path = 'logs/job_16082023_114400_filtered.txt'
    # log_path = '../logs/fjp_job_robin_true_largest_left_11092023_205700.txt'
    # log_path = '../logs/fjp_job_robin_true_popular_12092023_170800.txt'
    # log_path = '../logs/fjp_job_robin_true_smart_12092023_134200.txt'
    # log_path = '../logs/fjp_job_robin_true_default_14092023_185400.txt'
    # log_path = '../logs/fjp_canonical_stats_10092023_203700.txt'
    # log_path = '../logs/fjp_active_stats_10092023_203400.txt'
    # log_path = '../logs/fjp_fullyactive_stats_07092023_160700.txt'
    # log_path = 'logs/fjp_job_robin_true_sorted_12092023_155300.txt'

    # log_path = '../logs/fjp_canonical_stats_10092023_203700.txt'
    # log_path = '../logs/fjp_active_stats_10092023_203400.txt'
    log_path = '../logs/fjp_fullyactive_stats_07092023_160700.txt'

    data = parse(log_path)
    df = data_to_csv(data)
    df = df.set_index('query')
    with open('order_smart.txt', 'r') as f:
        l = list(map(lambda x: x.strip()[:-1], f.readlines()))
    df = df.loc[l].reset_index()
    # df = df[df['query'] != '19c']
    width = 7

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
    cax.set_ylabel('number of atoms', fontsize=13)
    ax.set_xlabel('query', fontsize=13)
    ax.set_ylabel('free join node index', fontsize=13)
    # plt.show()
    plt.tight_layout()
    fig.savefig('plots/fjp_colors_fullyactive.pdf')
    exit()
    bvals = [i-.5 for i in range(max_val+2)]
    colors = [matplotlib.colors.rgb2hex(cmap(i/(max_val))) for i in range(max_val+1)]
    scale = discrete_colorscale(bvals, colors)
    tickvals = [np.mean(bvals[k:k+2]) for k in range(len(bvals)-1)]
    ticktext = tickvals
    # fig = px.imshow(mat.T[::-1], color_continuous_scale=scale)
    # fig.update_coloraxes(
    #     colorbar = dict(tickvals=tickvals,
    #                     ticktext=ticktext)
    # )
    heatmap = go.Heatmap(z=mat.T,
        colorscale= scale,
        zmin=0,
        zmax=7,
        colorbar=dict(outlinecolor='black', 
                    outlinewidth=1,
                    orientation='v',
                    ticks='',
                    title='number of atoms',
                    titleside='right',
                    tick0=-0.5,
                    dtick=1,
                    )
    )
    fig = go.Figure(data=[heatmap])
    fig.update_xaxes(
            tickmode = 'array',
            tickvals = list(range(df.shape[0])),
            ticktext = df['query'],
            tickfont=dict(size=12),
    )

    fig.update_yaxes(
        title = 'free join node index',
    )
    fig.update_yaxes(showgrid=True, 
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    minor=dict(gridcolor='rgba(0,0,0,0.05)',showgrid=True),
                    gridcolor= "rgba(0,0,0,0.05)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                    )
    fig.update_xaxes(showgrid=True,
                     linewidth=0,
                     linecolor='black',
                     mirror=True,
                     showticklabels=True,
                     ticks='outside',
                     gridcolor= "rgba(0,0,0,0.05)",
                     titlefont=dict(size=18),
                     )
    fig.write_image('plots/fjp_colors_largest.pdf',scale=4, width=1400, height=400)
    # fig.show()