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

    data = []
    for size in [1e6, 5e6, 10e6]:
        for num_lists in [2**i for i in range(0, 8)]:
            data.append([num_lists, size/num_lists, str(int(size/1e6)) + ' M tuples'])

    df = pd.DataFrame(data=data, columns=['number of lists', 'avg number of entries per list', 'data size'])
    fig = px.line(df, x="number of lists",
                  y="avg number of entries per list",
                  color='data size',
                  log_x=True,
                  range_x=[0.9,142.22222222222217])
    
    fig.update_layout(
        legend=dict(
            title='',
            yanchor="bottom",
            font=dict(size=16),
            y=0.7,
            x=0.8,
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
                    type='log',
                    mirror=True,
                    showticklabels=True,
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    tickfont=dict(size=16),
                    titlefont=dict(size=18),
                     )
    fig.update_layout(
        xaxis = dict(
            tickmode = 'array',
            tickvals = [1, 2, 4, 8, 16, 32, 64, 128],
        )
    )
    
    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))
    fig.update_layout(legend_traceorder="normal")
    fig.write_image(
        "figures/plots/chunked_list.pdf",
        format="pdf",
        scale=4,
        width=800,
        height=400,
    )
    # fig.show()
