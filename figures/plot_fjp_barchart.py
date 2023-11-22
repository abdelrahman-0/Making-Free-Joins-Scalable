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
    data_path = os.path.join(
        os.path.dirname(__file__), 
        '..',
        'logs',
        # 'fjp_job_robin_true_largest_left_11092023_205700.txt',
        # 'fjp_job_robin_true_default_14092023_185400.txt',
        # 'fjp_job_robin_true_default_original_18092023_130000.txt',
        # 'fjp_job_robin_true_smart_12092023_134200.txt',
        # 'fjp_job_robin_true_smart_original_18092023_144800.txt',
        # 'fjp_job_robin_true_sorted_12092023_155300.txt',
        # 'fjp_job_robin_true_popular_12092023_170800.txt',
        # 'fjp_comparison.txt'
        # 'cll_vs_9d.txt',
        '9d_new.txt',
        # '10a.txt',
    )

    with open(data_path, 'r') as f:
        lines = f.readlines()

    operations = ('chunked list building', 'trie building', 'trie intersection')
    legend_dict = {
        'chunked list building' : 'chunked list building', 
        'trie building' : 'GHT building',
        'trie intersection' : 'GHT intersection'
    }
    query_str = 'Query: '
    nthread_str = 'Using: '

    query = ''
    nthreads = 0
    i = 0
    for l in lines:
        if l.startswith(query_str):
            # query = l[len(query_str):-1]
            query = f'2<sup>{i}</sup>'
            i+=1
        elif l.startswith(nthread_str):
            nthreads = int(l.split(' ')[1])
        elif l.startswith(operations):
            ls = l.split(',')
            op = legend_dict[ls[0]]
            time = float(ls[6]) * 1000 # ms
            data.append([query, nthreads, op, time, 0.0])

    metric = 'LLC misses'
    columns_names=['query', 'number of threads', 'operation', metric, 'speedup']
    df = pd.DataFrame(data=data, columns=columns_names)
    df = df[df['number of threads'] == 32]
    df = df.groupby(['query', 'operation'], as_index=False).sum()

    # df = df.sort_values(by=["operation","time (ms)"])
    # df = df.pivot(index='query',columns='operation', values='time (ms)')
    # df = df.reset_index()
    # df['sum'] = df['chunked list building'] + df['GHT building'] + df['GHT intersection']
    # df = df.sort_values(by='sum')
    # with open('order_smart_original.txt', 'w') as f:
    #     f.writelines(',\n'.join(df['query']))
    # exit()
    # df = df.melt(id_vars=['query'], value_vars=legend_dict.values(),
    #              var_name='operation', value_name='time (ms)')

    ###############################
    # with open('order_smart_original.txt', 'r') as f:
    #     l = list(map(lambda x: x.strip()[:-1], f.readlines()))
    # df = df.set_index('query')
    # df = df.loc[l].reset_index()
    ###############################
    fig = px.bar(df, x='query', y=metric, color='operation',
                 category_orders={'operation':legend_dict.values(),
                                  'query' : [f'2<sup>{i}</sup>' for i in range(19)]},
                 color_discrete_sequence=PLOTLY_COLORS[:3])

    ###############################


    # fig = go.Figure()
    # for j, op in enumerate(legend_dict.values()):
    #     df_op = df[df['operation'] ==  op]
    #     fig.add_trace(go.Bar(
    #             x=df_op['query'],
    #             y=df_op['time (ms)'],
    #             marker_color=PLOTLY_COLORS[j], # marker color can be a single color value or an iterable
    #             name=op,
    #         ))
    fig.update_layout(barmode='stack')
    fig.update_traces(marker_line_width=0.5, marker_line_color='rgb(0,0,0)')


    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            font=dict(size=16),
            x=0.75,
            y=0.73,
            traceorder='reversed',
            orientation='v',
            bgcolor='rgba(0,0,0,0)',
            ),
            template='plotly_white',
            )

    fig.update_yaxes(showgrid=True, 
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    title=f'{metric}',
                    gridcolor= "rgba(0,0,0,0.2)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                    # range=[0,1600],
                    )
    fig.update_xaxes(showgrid=False,
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    tickmode='linear',
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.07)",
                    title='number of paritions',
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                     )
    
    fig.update_xaxes(tickangle=-90)
    
    save_path = os.path.join(
        os.path.dirname(__file__), 
        '..',
        'figures',
        'plots',
        'queries_9d_llc.pdf',
    )
    fig.write_image(
        save_path,
        format='pdf',
        scale=4,
        width=750,
        height=500,
    )
    # fig.show()
