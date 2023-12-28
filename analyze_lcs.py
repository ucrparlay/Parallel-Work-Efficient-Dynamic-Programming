

def go(n):
  if n <= 100:
    return str(n)
  for i in range(20):
    if n == 10**i:
      return '1e' + str(i)
  return str(n)


def last_num(s):
    s = s.strip()
    t = s.split(' ')[-1]
    try:
      return float(t)
    except:
      return 0


def work(n, m):
  f = open(f'./tmp/n_{go(n)}_m_{go(m)}.txt')
  lines = f.readlines()
  f.close()
  
  ks = []
  ps = []
  ss = []
  for line in lines:
    x = last_num(line)
    if line.startswith('k:'):
      ks.append(int(x))
    if line.startswith('Parlay time: parallel:'):
      ps.append(x)
    if line.startswith('Parlay time: sequential:'):
      ss.append(x)
  print('\nn:', go(n), ' m:', go(m))
  print(ks)
  print(ps)
  print(ss)


if __name__ == '__main__':
  for n in [10**7, 10**8, 10**9]:
    for m in [10**7, 10**8, 10**9]:
      work(n, m)