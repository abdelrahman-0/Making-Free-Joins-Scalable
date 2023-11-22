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
pd.options.display.max_columns = None

if __name__ == '__main__':

    # load data
    base_path = os.path.join(
        os.path.dirname(__file__), 
        '..',
        'logs',
    )
    operations = ('chunked list building', 'trie building', 'trie intersection')
    legend_dict = {
        'chunked list building' : 'chunked list building', 
        'trie building' : 'GHT building',
        'trie intersection' : 'GHT intersection'
    }
    query_str = 'Query: '
    nthread_str = 'Using: '
    columns_names=['query', 'number of threads', 'operation', 'time (ms)']
    dfs = []
    paths = [
                # 'fjp_job_robin_true_largest_left_11092023_205700.txt',
                'fjp_job_robin_true_default_14092023_185400.txt',
                # 'fjp_job_robin_true_default_original_18092023_130000.txt',
                # 'fjp_job_robin_true_smart_12092023_134200.txt',
                'fjp_job_robin_true_smart_original_18092023_144800.txt',
                # 'fjp_job_robin_true_sorted_12092023_155300.txt',
                # 'fjp_job_robin_true_popular_12092023_170800.txt',
            ]
    for path in paths:
        data = []
        data_path = os.path.join(base_path, path)
        with open(data_path, 'r') as f:
            lines = f.readlines()

        query = ''
        nthreads = 0
        i = 0
        for l in lines:
            if l.startswith(query_str):
                query = l[len(query_str):-1]
                i+=1
            elif l.startswith(nthread_str):
                nthreads = int(l.split(' ')[1])
            elif l.startswith(operations):
                ls = l.split(',')
                op = legend_dict[ls[0]]
                time = float(ls[1]) * 1000 # ms
                data.append([query, nthreads, op, time])

        df = pd.DataFrame(data=data, columns=columns_names)
        df = df[df['number of threads'] == 32]
        df = df[df['query'] != '19c']
        df = df.groupby(['query', 'operation'], as_index=False).sum()
        dfs.append(df)
    dfs[0].set_index(['query', 'operation'])
    dfs[1].set_index(['query', 'operation'])
    join_df = dfs[0].join(dfs[1], lsuffix='_default', rsuffix='_smart')
    tick_dict = {
        'chunked list building' : np.arange(0,200,25),
        'GHT building' : np.arange(0,1800,400),
        'GHT intersection' : np.arange(0,2000,400),
        'sum' : np.arange(0,2600,500),
    }
    fig = make_subplots(rows=2, cols=2, shared_xaxes=False, shared_yaxes=False, print_grid=False)
    for i, op in enumerate(legend_dict.values()):
        mask = join_df['operation_default'] == op
        x = join_df[mask]['time (ms)_default']
        y = join_df[mask]['time (ms)_smart']
        max_val = max(x.max(), y.max())
        fig.add_trace(go.Scatter(x=x, y=y, mode='markers', marker=dict(color=PLOTLY_COLORS[i]),
                      name=op,
                      xaxis=f"x{i+1}",),
                      row=(i//2)+1, col=(i%2)+1)
        fig.add_shape(type="line",
                    line=dict(width=1),
                    x0=0,
                    y0=0,
                    x1=max_val,
                    y1=max_val,
                    row=(i//2)+1, col=(i%2)+1)
        fig.update_xaxes(title=paths[0],
                        row=(i//2)+1, col=(i%2)+1,
                        showgrid=False,
                        linewidth=1,
                        linecolor='black',
                        mirror=True,
                        showticklabels=True,
                        ticks='outside',
                        minor=dict(gridcolor='rgba(0,0,0,0.05)',showgrid=False),
                        gridcolor= "rgba(0,0,0,0.1)",
                        tickfont=dict(size=16),
                        titlefont=dict(size=18),
                        range=[0,max_val],
                        tickmode='array',
                        tickvals=tick_dict[op],
                        ticktext=tick_dict[op],
                        title_standoff=0,
                        )
        
        fig.update_yaxes(title=paths[1],
                        showgrid=False,
                        linewidth=1,
                        linecolor='black',
                        mirror=True,
                        showticklabels=True,
                        ticks='outside',
                        gridcolor= "rgba(0,0,0,0.1)",
                        tickfont=dict(size=16),
                        titlefont=dict(size=18),
                        range=[0,max_val],row=(i//2)+1, col=(i%2)+1,
                        # scaleanchor=f"x{i+1}",
                        # scaleratio=1,
                        tickmode='array',
                        tickvals=tick_dict[op],
                        ticktext=tick_dict[op],
                        title_standoff=0,
                        )
    fig.update_layout(template='plotly_white')
    
    x = df_sum = join_df.groupby(['query_default'], as_index=False).sum()['time (ms)_default']
    y = df_sum2 = join_df.groupby(['query_smart'], as_index=False).sum()['time (ms)_smart']
    max_val = max(x.max(), y.max())
    gray = 0.6
    fig.add_trace(go.Scatter(x=x, y=y, mode='markers', marker=dict(color=f'rgb({gray},{gray},{gray})'),
                      name='total'), row=2, col=2)
    fig.add_shape(type="line",
                line=dict(width=1),
                x0=0, 
                y0=0, 
                x1=max_val, 
                y1=max_val,
                row=2, col=2)
    fig.update_xaxes(title=paths[0],
                    row=2, col=2,
                    showgrid=False,
                    linewidth=1,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    gridcolor="rgba(0,0,0,0.1)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                    range=[0,max_val],
                    title_standoff=0,
                    )

    fig.update_yaxes(title=paths[1],
                    showgrid=False,
                    linewidth=1,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                    range=[0,max_val],
                    row=2, col=2,
                    scaleanchor="x5",
                    scaleratio=1,
                    title_standoff=0,
                    )
    
    fig. update_layout(showlegend=True)
    fig.update_layout(
        legend=dict(
            bordercolor="Black",
            borderwidth=1,
            orientation='h',
            xanchor="center",
            x=0.5,
        )
    )
    
    save_path = os.path.join(
        os.path.dirname(__file__), 
        '..',
        'figures',
        'plots',
        'queries_comp_old.pdf',
    )


    fig.write_image(
        save_path,
        format='pdf',
        scale=4,
        width=1000,
        height=1050,
    )
    # fig.show()
