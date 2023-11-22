import numpy as np

import plotly.graph_objects as go
import pandas as pd
import plotly.io as pio
import plotly.express as px
pio.kaleido.scope.mathjax = None

if __name__ == '__main__':

    # load data
    llc_misses = [
        43843819.471894,
        45843201.845835,
        46016075.338494,
        50378281.895096,
    ]
    structures = [
        'hash table', 
        'GHT-1', 
        'GHT-2',
        'hash trie',
    ]

    colors = [
        '#1f77b4',  # muted blue
        '#d62728',  # brick red
        '#2ca02c',  # cooked asparagus green
        '#9467bd',  # muted purple
    ]

    data = []

    for x,y,z in zip(structures, llc_misses, colors):
        data.append([x,y,z])


    # fig = go.Figure(data=[go.Bar(
    #         x=structures,
    #         y=llc_misses,
    #         marker_color=colors,
    #         width=[0.5] * 4
    #     )], width=400, height=400)

    df = pd.DataFrame(data=data, columns=['structure', 'llc', 'color'])


    fig = px.bar(df, x='structure', y='llc', color='structure', width=400, height=400)
    
    fig.update_yaxes(showgrid=True, 
                    linewidth=0,
                    linecolor='black',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    title='LLC miss counts',
                    gridcolor= "rgba(0,0,0,0.1)",
                    )
    fig.update_xaxes(showgrid=True,
                     linewidth=0,
                     linecolor='black',
                     mirror=True,
                     showticklabels=True,
                     ticks='outside',
                     title='',
                     gridcolor= "rgba(0,0,0,0.1)",
                     )
    fig.update_layout(
        xaxis = dict(
            tickmode = 'linear',
            tick0 = 0,
            dtick = 1
        ),
        template='plotly_white'
    )
    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))
    
    fig.update_yaxes(range=[40e6, 51e6])
    fig.write_image(
        "figures/plots/benchmark_indexes_llc_misses.pdf",
        format="pdf",
        scale=4,
        width=450,
        height=400,
    )

