import numpy as np

import plotly.graph_objects as go
import pandas as pd
import plotly.io as pio
import plotly.express as px
pio.kaleido.scope.mathjax = None # needed to remove tiny text at the bottom of figure
import re
import os
from plotly_colors import PLOTLY_COLORS

if __name__ == '__main__':

    # load data
    data = []
    structures = {
        'hashtable' :   'hash table  (depth=1)', 
        'ght-1' :       'GHT-1         (depth=2)', 
        'ght-2' :       'GHT-2         (depth=2)',
        'hashtrie':     'hash trie     (depth=3)',
    }

    structures_list = list(structures.keys())
    columns=['Number of Threads', 'time (ms)', 'datastructure']

    data_path = os.path.join(os.path.dirname(__file__), '..', 'logs', 'bm_indexes_11092023_120000.txt')

    with open(data_path, 'r') as f:
        lines = f.readlines()

    for i in range(0, len(lines)-9, 9):

        # prepare dataframe
        n_threads = int(re.split(' ', lines[i])[1])
        for j in range(4,8):
            l = lines[i+j]
            split = re.split(',', lines[i+j])
            struct = structures[split[0].strip()]
            time = float(split[1]) * 1000
            l1 = float(split[5])
            llc = float(split[6])
            color = PLOTLY_COLORS[j-4]
            # data.append([n_threads, struct, 'time', time, color])
            data.append([n_threads, struct, 'L1 misses', l1, color])
            data.append([n_threads, struct, 'LLC misses', llc, color])

    columns_names=['number of threads', 'data structure', 'metric', 'total misses', 'color']
    df = pd.DataFrame(data=data, columns=columns_names)

    # fig = go.Figure()
    # for j,(k,s) in enumerate(structures.items()):
    #     df_tmp = df[df['data structure'] == s]
    #     fig.add_trace(go.Bar(
    #         x=df_tmp['metric'],
    #         y=df_tmp['total misses'],
    #         marker_color = PLOTLY_COLORS[j],
    #         name=k,
    #     ))
    df = df[df['number of threads'] == 1]
    figure_data = []

    for j,(k,i) in enumerate(structures.items()):
        df_tmp = df[df['data structure'] == i]
        figure_data.append(go.Bar(name = i,
                                x = df_tmp['metric'],
                                y = df_tmp['total misses'],
                                text=[str(round((x/1e6), 1)) + ' M' for x in df_tmp['total misses']],
                                textfont=dict(size=16),
                                marker=dict(color=df_tmp['color'])))
            
    fig = go.Figure(data=figure_data, width=800,height=400)
    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            font=dict(size=16),
            y=0.7,
            x=0.7,
            bgcolor='rgba(0,0,0,0)',
            ),
            template='plotly_white')
    
    fig.update_yaxes(showgrid=True,
                    title='total misses',
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                    )
    fig.update_xaxes(showgrid=True,
                     linewidth=0,
                     linecolor='black',
                     mirror=True,
                     showticklabels=True,
                     ticks='outside',
                     gridcolor= "rgba(0,0,0,0.1)",
                     tickfont=dict(size=16),
                    titlefont=dict(size=18),
                     )
    # fig.update_layout(
    #     xaxis = dict(
    #         tickmode = 'array',
    #         tickvals = [1, 2, 4, 8, 16, 32],
    #         # tick0 = 0,
    #         # dtick = 1
    #     )
    # )
    
    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))
    fig.update_layout(legend_traceorder="normal")
    # fig.show()
    fig.write_image(
        "figures/plots/bechmark_indexes_cache_11092023_145500.pdf",
        format="pdf",
        scale=4,
        width=800,
        height=400,
    )
