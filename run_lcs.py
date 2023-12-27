import subprocess

if __name__ == '__main__':
  n = 100000000
  m = 100000000
  for k in [1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000]:
    if m <= k * n:
      subprocess.call(f'./build/lcs -run par -n {n} -m {m} -k {k}', shell=True)