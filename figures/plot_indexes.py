import numpy as np

import plotly.graph_objects as go
import pandas as pd
import plotly.io as pio
import plotly.express as px
pio.kaleido.scope.mathjax = None

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
    
    data_paths = [
        'logs/benchmark_indexes_31082023_213000.txt',
        'logs/benchmark_indexes_31082023_223800.txt',
        'logs/benchmark_indexes_31082023_223900.txt',
        'logs/benchmark_indexes_31082023_224000.txt',
        'logs/benchmark_indexes_31082023_224100.txt',
    ]

    for data_path in data_paths:
        # prepare dataframe
        with open(data_path, 'r') as f:
            lines = f.readlines()

        idxs = list(map(lambda x: ':' in x and x[:x.index(':')] in columns+structures_list, lines))
        lines = np.array(lines)[idxs]

        for i in range(0, len(lines), 5):
            raw_str = lines[i].strip()
            raw_str = raw_str[len(columns[0])+1:]
            tup = [int(raw_str)]
            for j in range(1,5):
                raw_str = lines[i+j].strip()
                raw_str = raw_str[len(structures_list[j-1])+1:]
                data.append(tup + [int(raw_str), structures[structures_list[j-1]]])
        break
    # plot boxplot
    columns_names=['Number of Threads', 'time (ms)', 'data structure']
    df = pd.DataFrame(data=data, columns=columns_names)
    
    # fig = px.box(df, x="Number of Threads", y="time (ms)", color="datastructure")
    fig = px.line(df, x="Number of Threads", y="time (ms)", color="data structure", markers=True,
                  width=800, height=400)
    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            y=0.7,
            x=0.75,
            bgcolor='rgba(0,0,0,0)'),
            template='plotly_white')
    
    fig.update_yaxes(showgrid=True, 
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    )
    fig.update_xaxes(showgrid=True,
                     linewidth=0,
                     linecolor='black',
                     mirror=True,
                     showticklabels=True,
                     ticks='outside',
                     gridcolor= "rgba(0,0,0,0.1)",
                     )
    fig.update_layout(
        xaxis = dict(
            tickmode = 'linear',
            tick0 = 0,
            dtick = 1
        )
    )
    
    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))
    fig.write_image(
        "figures/plots/bechmark_indexes.pdf",
        format="pdf",
        scale=4,
        width=800,
        height=400,
    )

