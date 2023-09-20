

def last_num(s):
    s = s.strip()
    t = s.split(' ')[-1]
    return float(t)


def main():
    path = 'logs/1e7.txt'
    file = open(path, 'r')
    lines = file.readlines()
    file.close()
    ids = []
    for i in range(len(lines)):
        if lines[i].startswith('Post Office'):
            ids.append(i)
    ids.append(len(lines))
    for i in range(len(ids) - 1):
        part = lines[ids[i]: ids[i+1]]
        cost = 0
        seq = 0
        par = 0
        steps = ''
        for line in part:
            if line.startswith('cost:'):
                cost = last_num(line)
            if line.startswith('Parlay time: sequential:'):
                seq = last_num(line)
            if line.startswith('Parlay time: parallel:'):
                par = last_num(line)
            if line.startswith('step:'):
                if steps != '':
                    steps += ', '
                steps += '('
                steps += line.strip()
                steps += ')'
        print(cost, seq, par, steps, sep=';')


if __name__ == '__main__':
    main()
