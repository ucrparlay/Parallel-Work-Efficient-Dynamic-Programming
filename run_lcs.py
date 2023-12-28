import subprocess

def go(n):
  if n <= 100:
    return str(n)
  for i in range(20):
    if n == 10**i:
      return '1e' + str(i)
  return str(n)

if __name__ == '__main__':
  log_dir = 'logs_1228'
  subprocess.call(f'mkdir -p {log_dir}', shell=True)
  for n in [10**8, 10**9]:
    for m in [10**8, 10**9]:
      name = f'n_{go(n)}_m_{go(m)}'
      for k in [1, 10, 100, 1000, 10**4, 10**5, 10**6, 10**7, 10**8, 10**9, 10**10]:
        if k <= m and k <= n and m <= k * n:
          cmd = f'./build/lcs -run par,seq -n {n} -m {m} -k {k}'
          subprocess.call(f'{cmd} &>>{log_dir}/{name}.txt', shell=True)
