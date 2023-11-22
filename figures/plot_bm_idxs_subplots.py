import numpy as np

import plotly.graph_objects as go
import pandas as pd
import plotly.io as pio
import plotly.express as px
from plotly.subplots import make_subplots
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
            data.append([n_threads, struct, 'time', time, color])
            data.append([n_threads, struct, 'L1 misses', l1, color])
            data.append([n_threads, struct, 'LLC misses', llc, color])

    columns_names=['number of threads', 'data structure', 'metric', 'value', 'color']
    df = pd.DataFrame(data=data, columns=columns_names)

    num_tuples = 2963664
    fig = make_subplots(rows=1, cols=2)
    for i,s in enumerate(structures.values()):
        df_struct = df[df['data structure'] == s]
        df_struct = df_struct[df_struct['metric'] == 'time']
        fig.add_trace(go.Scatter(x=df_struct['number of threads'], y=num_tuples/df_struct['value'],
                    name=s,
                    line=dict(color=df_struct['color'].iloc[0]),
                    connectgaps=True,
                    mode="lines+markers",
                    showlegend=True,
                    legendgroup='group1',
                ),
                row=1, 
                col=1,
            )
        
    df = df[df['number of threads'] == 1]
    df = df[df['metric'] != 'time']
    for j,(k,i) in enumerate(structures.items()):
        df_tmp = df[df['data structure'] == i]
        fig.add_trace(go.Bar(name = i,
                            x = df_tmp['metric'],
                            y = df_tmp['value'],
                            text=[str(round((x/1e6), 1)) + ' M' for x in df_tmp['value']],
                            textfont=dict(size=16),
                            marker=dict(color=df_tmp['color']),
                            showlegend=False,
                            legendgroup='group1',
                            ),
                    row=1, 
                    col=2,)
    
    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            font=dict(size=16),
            y=0.3,
            x=0.3,
            orientation='v',
            bgcolor='rgba(0,0,0,0)'),
            template='plotly_white')
    
    fig.update_yaxes(showgrid=True, 
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
                     tickfont=dict(size=18),
                    titlefont=dict(size=18),
                     )
    fig.update_layout(
        xaxis = dict(
            tickmode = 'array',
            tickvals = [1, 2, 4, 8, 16, 32],
        )
    )
    
    fig['layout']['xaxis']['title']='number of threads'
    fig['layout']['yaxis']['title']='throughput (tuples/s)'
    fig['layout']['yaxis2']['title']='total misses'

    # fig['layout']['xaxis']['title']='number of threads'
    # fig['layout']['yaxis']['title']='time (ms)'
    # fig['layout']['yaxis2']['title']='total misses'

    for axis in ['xaxis', 'yaxis', 'xaxis2', 'yaxis2']:
        fig['layout'][axis]['titlefont']=dict(size=18)

    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))
    # fig.write_image(
    #     "plots/bechmark_indexes_new.pdf",
    #     format="pdf",
    #     scale=4,
    #     width=1700,
    #     height=450,
    # )
    fig.show()
