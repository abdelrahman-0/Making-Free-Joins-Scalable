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
    data_path = os.path.join(os.path.dirname(__file__), 
                            '..', 
                            'logs', 
                            # 'fjp_job_robin_true_largest_left_11092023_205700.txt',
                            # 'fjp_job_robin_true_default_14092023_185400.txt',
                            # 'fjp_job_robin_true_default_original_18092023_130000.txt',
                            # 'fjp_job_robin_true_smart_12092023_134200.txt',
                            'fjp_job_robin_true_smart_original_18092023_144800.txt',
                            # 'fjp_job_robin_true_popular_12092023_170800.txt',
                            )

    with open(data_path, 'r') as f:
        lines = f.readlines()

    operations = ('chunked list building', 'trie building', 'trie intersection')
    legend_dict = {
        'chunked list building' : 'chunked list building', 
        'trie building' : 'GHT building',
        'trie intersection' : 'GHT intersection',
    }
    query_str = 'Query: '
    nthread_str = 'Using: '

    query = ''
    nthreads = 0
    for l in lines:
        if l.startswith(query_str):
            query = l[len(query_str):-1]
        elif l.startswith(nthread_str):
            nthreads = l.split(' ')[1]
        elif l.startswith(operations):
            ls = l.split(',')
            op = legend_dict[ls[0]]
            time = float(ls[1]) * 1000 # ms
            data.append([query, nthreads, op, time])

    columns_names=['query', 'number of threads', 'operation', 'time (ms)']
    df = pd.DataFrame(data=data, columns=columns_names)

    df = df.groupby(['query', 'number of threads', 'operation'], as_index=False).sum()
    df = df.pivot(index=['query', 'number of threads'], columns='operation', values='time (ms)')
    df['total'] = 0.0
    for op in legend_dict.values():
        df['total'] += df[op]
    df = df.reset_index(['query', 'number of threads'])
    # df = df[df['number of threads'] == '32']
    df = pd.melt(df, id_vars=['query', 'number of threads'], value_vars=list(legend_dict.values())+['total'], var_name='operation', value_name='time (ms)')

    # df.reset_index(level=['query', 'number of threads'])
    speedup_dict = {}
    for q in df['query'].unique():
        df_q = df[df['query'] == q]
        df_base = df_q[df_q['number of threads'] == '1']
        for op in list(legend_dict.values())+['total']:
            df_op = df_base[df_base['operation'] == op]
            base = df_op['time (ms)'].iloc[0]
            speedup_dict[(q, op)] = base
    df['speedup'] = df.apply(lambda row: speedup_dict[(row['query'], row['operation'])] / (row['time (ms)'] / int(row['number of threads'])), axis=1)
    # df['speedup'] = df['number of threads'].apply(int) / df['time (ms)']
    
    gray=0.6
    colors = PLOTLY_COLORS[:3]+['#999999']
    fig = px.box(df, x='number of threads', y='speedup', 
                 color='operation', boxmode='group',
                 category_orders={'operation': list(legend_dict.values())+['total']},
                 color_discrete_sequence=colors, log_y=True)
    
    for i, data in enumerate(fig.data):
        data['line'] = dict(color='black', width=1.5)
        data['fillcolor'] = colors[i]

    fig.update_xaxes(
        categoryorder='array',
        categoryarray= ['1', '2', '4', '8', '16', '32']
    )

    # fig.update_xaxes(type='log')
    # fig = go.Figure()
    # deltas = [-1,0,1]
    # for i,op in enumerate(operations):
    #     df_op = df[df['operation'] == op]
    #     fig.add_trace(
    #         go.Box(
    #             x=df_op['number of threads'],
    #             y=df_op['speedup'],
    #             name=op,
    #     ))
    
    
    # fig.update_layout(
    #     xaxis = dict(
    #         tickmode = 'array',
    #         tickvals = [1, 2, 4, 8, 16, 32],
    #     )
    # )
    
    
    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            font=dict(size=16),
            x=0.02,
            y=0.73,
            orientation='v',
            bgcolor='rgba(0,0,0,0)',),
            template='plotly_white')
    
    fig.update_yaxes(showgrid=True, 
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    title='speedup per thread',
                    gridcolor= "rgba(0,0,0,0.2)",
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
                     tickfont=dict(size=18),
                     titlefont=dict(size=18),
                     )
    
    fig.write_image(
        'figures/plots/boxplot_speedup_per_thread.png',
        format='png',
        scale=4,
        width=1400,
        height=550,
    )
    # fig.show()
