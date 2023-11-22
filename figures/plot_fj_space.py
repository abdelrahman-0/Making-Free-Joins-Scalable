import plotly.express as px
import pandas as pd
import plotly.graph_objects as go

def parse(path: str):
    with open(path, 'r') as f:
        lines = f.readlines()
    
    current_query = ''
    n_threads = -1
    data = []
    i = 0
    while i < len(lines):
        l = lines[i].strip()
        if l.startswith('Data'):
            current_query = l[l.rindex('/')+1:]
            i += 1
        if l.startswith('Using:'):
            n_threads = int(l.split(' ')[1])
            i += 1
        elif l.startswith('Num ') and 'Tuples' not in l:
            l2 = lines[i+1].strip()
            l3 = lines[i+2].strip()
            num_nodes = int(l[l.index(':')+1:].strip())
            num_atoms = list(map(lambda x: int(x.strip()), l2[l2.index(':')+1:].split(' ')))
            num_attributes = list(map(lambda x: int(x.strip()), l3[l3.index(':')+1:].split(' ')))
            data += [[current_query, n_threads, num_nodes, num_atoms, num_attributes]]
            i += 3
        else:
            i += 1
    return data

def data_to_csv(data):
    raw_df = []
    for d in data:
        if d[1] != 1:
            continue
        q = d[0]
        for (natom, nattr) in zip(d[3], d[4]):
            raw_df += [[q, natom, nattr]]
    df = pd.DataFrame(data=raw_df, columns=['query', 'num atoms', 'num attributes'])
    return df


if __name__ == '__main__':
    # read data
    # log_path = 'logs/job_16082023_114400_filtered.txt'
    # log_path = '../logs/fjp_job_robin_true_largest_left_11092023_205700.txt'
    # log_path = '../logs/fjp_job_robin_true_popular_12092023_170800.txt'
    log_path = '../logs/fjp_job_robin_true_smart_12092023_134200.txt'
    # log_path = '../logs/fjp_canonical_stats_10092023_203700.txt'
    # log_path = '../logs/fjp_active_stats_10092023_203400.txt'
    # log_path = '../logs/fjp_fullyactive_stats_07092023_160700.txt'
    # log_path = 'logs/fjp_job_robin_true_sorted_12092023_155300.txt'
    
    data = parse(log_path)
    df = data_to_csv(data)
    x = df['num atoms']
    y = df['num attributes']

    # fig = px.line(df, x='num atoms', y='num attributes', color='query')

    # fig = go.Figure(go.Histogram2dContour(
    #         x = df['num atoms'],
    #         y = df['num attributes'],
    #         colorscale = 'Blues'
    # ))
    
    # fig.add_trace(go.Scatter(x=df['num atoms'], y=df['num attributes'], mode='markers', marker_color='rgba(146, 137, 163, .9)'))

    fig = go.Figure(go.Histogram2d(
        x=x,
        y=y,
        texttemplate= "%{z}",
        colorscale='Blues',
        zmin=0,
        zmax=260,
        xbins=dict(start=0.5, end=8.5, size=1),
        ybins=dict(start=0.5, end=5.5, size=1),
        colorbar=dict(outlinecolor='black', 
                        outlinewidth=1,
                        title='Node Frequency',
                        orientation='v',
                        ticks='inside',
                        titleside='right'),
    ))

    fig.update_xaxes(title_text='Atoms',
                    tickmode = 'array',
                    tickvals = list(range(1,9)))
    
    fig.update_yaxes(title_text='Attributes',
                    tickmode = 'array',
                    tickvals = list(range(1,6)))
    

    # fig.update_layout(showlegend=False)
    # fig.update_coloraxes(showscale=False)
    fig.update_layout(title='Free Join Space', title_x=0.5)
    fig.update_xaxes(showgrid=True, linewidth=1, linecolor='black', mirror=True, showticklabels=True)
    fig.update_yaxes(showgrid=True, linewidth=1, linecolor='black', mirror=True, showticklabels=True)
    # fig.write_image('figures/job_fj_space.pdf',scale=6, width=700, height=450)
    fig.show()