

def last_num(s):
    s = s.strip()
    t = s.split(' ')[-1]
    return float(t)


def work(name):
    path = f'logs_1217/{name}_seq.txt'
    print(path)
    file = open(path, 'r')
    lines = file.readlines()
    file.close()
    ids = []
    for i in range(len(lines)):
        if lines[i].startswith('Post Office'):
            ids.append(i)
    ids.append(len(lines))
    seq0 = []
    par0 = []
    k0 = []
    for i in range(len(ids) - 1):
        part = lines[ids[i]: ids[i+1]]
        cost = 0
        seq = 0
        par = 0
        k = 0
        steps = ''
        for line in part:
            if line.startswith('cost:'):
                cost = last_num(line)
            if line.startswith('Parlay time: sequential:'):
                seq = last_num(line)
            if line.startswith('Parlay time: new2:'):
                par = last_num(line)
            if line.startswith('output size:'):
                k = last_num(line)
            if line.startswith('step:'):
                if steps != '':
                    steps += ', '
                steps += '('
                steps += line.strip()
                steps += ')'
        # print(cost, seq, par, k, steps, sep=';')
        seq0.append(str(seq))
        par0.append(str(par))
        k0.append(str(k))
    print(','.join(seq0))
    print(','.join(par0))
    print(','.join(k0))


if __name__ == '__main__':
    for name in ['1e8','1e9']:
        work(name)

