import math
import numpy as np
from scipy.optimize import linprog
from itertools import combinations

import plotly.graph_objects as go
import pandas as pd
import plotly.io as pio   
pio.kaleido.scope.mathjax = None


N = 10


def get_l1_matrix(n, m):
    mat = np.zeros((m, m*n))
    for i in range(m):
        mat[i, i*n:(i+1)*n] = np.ones(n)
    return mat



def get_projection_matrix_lw(n, m):
    mat = np.zeros((m*n, n))
    for i in range(m):
        diag = np.ones(n)
        diag[-i-1] = 0
        mat[i*n:(i+1)*n] = np.diag(diag)
    return mat

def get_projection_matrix_cliques(n, m):
    mat = np.zeros((m*n, n))
    combs = list(combinations(list(range(m)), 2))
    for i in range(m):
        diag = np.zeros(n)
        idxs = [j for j in range(len(combs)) if i in combs[j]]
        diag[idxs] = 1
        mat[i*n:(i+1)*n] = np.diag(diag)
    return mat

def get_projection_matrix_cycle(n, m):
    mat = np.zeros((m*n, n))
    for i in range(m-1):
        diag = np.zeros(n)
        idxs = [i, i+1]
        diag[idxs] = 1
        mat[i*n:(i+1)*n] = np.diag(diag)
    diag = np.zeros(n)
    idxs = [0, n-1]
    diag[idxs] = 1
    mat[-n:] = np.diag(diag)
    return mat


def get_projection_matrix_star(n, m):
    mat = np.zeros((m*n, n))
    for i in range(m):
        diag = np.zeros(n)
        idxs = [0, i+1]
        diag[idxs] = 1
        mat[i*n:(i+1)*n] = np.diag(diag)
    return mat


def solve_agm_bound(n, m, topology='clique'):
    M = get_l1_matrix(n, m)
    if topology == 'clique':
        P = get_projection_matrix_cliques(n, m)
    elif topology == 'lw':
        P = get_projection_matrix_lw(n, m)
    elif topology == 'cycle':
        P = get_projection_matrix_cycle(n, m)
    elif topology == 'star':
        P = get_projection_matrix_star(n, m)
    res = linprog(np.full(n, math.log2(N)), -np.matmul(M, P), np.full(m, -1), integrality=0)
    bound = 1.0
    for val in res.x:
        bound *= (N ** val)
    # print(f'n={n}, m={m} ({res.success}): {bound}, {res.x}')
    return bound


if __name__ == '__main__':
    
    xs = []
    ys_clique = []
    ys_lw = []
    ys_cycle = []
    ys_cross = []
    ys_star = []

    for i in range(3, 16):
        n = math.comb(i, 2)
        xs.append(n)
        ys_clique.append(solve_agm_bound(n, i, 'clique'))
        ys_lw.append(solve_agm_bound(n, n, 'lw'))
        ys_cycle.append(solve_agm_bound(n, n, 'cycle'))
        ys_star.append(solve_agm_bound(n, n-1, 'star'))
        ys_cross.append(N**n)

    fig = go.Figure()

    # fig.add_trace(go.Scatter(x=xs, y=ys_cross,
    #                     mode='lines+markers',
    #                     name='Cross Product'))
    
    fig.add_trace(go.Scatter(x=xs, y=ys_cycle,
                        mode='lines+markers',
                        name='Cycle'))
    
    fig.add_trace(go.Scatter(x=xs, y=ys_clique,
                        mode='lines+markers',
                        name='Attribute Clique'))
    
    fig.add_trace(go.Scatter(x=xs, y=ys_lw,
                        mode='lines+markers',
                        name='Relation Clique (Loomis-Whitney)'))
    
    fig.add_trace(go.Scatter(x=xs, y=ys_star,
                        mode='lines+markers',
                        marker_symbol='star',
                        name='Star',
                        line=dict(width=1.2,color='#ff7f0e'),
                        ))
    
    fig.update_yaxes(exponentformat = 'power')
    # fig.add_hline(y=N, line_dash="dash", fillcolor='black', line_width=2)
    # fig.add_annotation(x=5, y=.3,
    #         text="relation size",
    #         showarrow=False,
    #         yshift=0)
    fig.update_layout(legend=dict(
        orientation="v",
        yanchor="bottom",
        y=0.7,
        x=0.02,
        bgcolor='rgba(0,0,0,0)'),
        template='plotly_white')
        # xanchor="center",
    fig.update_layout(margin=dict(l=65, r=10, b=10, t=10))

    fig.update_layout(
        xaxis_title="Number of Relations",
        yaxis_title="log<sub>10</sub>(AGM bound)",
    )

    fig.update_yaxes(type="log",
                    tickfont=dict(size=12),
                    showgrid=True,
                    linewidth=0, 
                    linecolor='black', 
                    mirror=True, 
                    showticklabels=True, 
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    )
    fig.update_xaxes(
                    tickfont=dict(size=14),
                    showgrid=True,
                    linewidth=0, 
                    linecolor='black', 
                    mirror=True, 
                    showticklabels=True, 
                    ticks='outside',
                    gridcolor= "rgba(0,0,0,0.1)",
                    )

    # fig.update_yaxes(type="log", tickfont=dict(size=10), showgrid=True, showticklabels=True, ticks='outside')
    # fig.update_xaxes(tickfont=dict(size=12), showgrid=True, showticklabels=True, ticks='outside')

    colors = [
    '#1f77b4',  # muted blue
    '#d62728',  # brick red
    '#2ca02c',  # cooked asparagus green
    '#ff7f0e',  # safety orange
    ]

    text_offsets = [53.0, 8.0, 3.1, 0.2]

    for i, l in enumerate([ys_cycle, ys_clique, ys_lw, ys_star]):
        t = round(math.log10(l[-1]), 2)
        fig.add_annotation(x=110, y=text_offsets[i],
                text=f"10<sup>{t}</sup>",
                showarrow=False,
                yshift=0,
                font=dict({'color':colors[i]}))


    fig.write_image(
        "figures/plots/agm_bound.pdf",
        format="pdf",
        scale=4,
        width=800,
        height=400,
    )
