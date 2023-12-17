
import subprocess


def main():
    log_dir = 'logs_1217'
    config = [
        # (10**6, '1e6'),
        # (10**7, '1e7'),
        (10**8, '1e8'),
        (10**9, '1e9'),
    ]
    for n, name in config:
        rrr = 10000000000
        costs = [0]
        for t in range(20):
            costs.append(10**t)
        for cost in costs:
            cmd = f'./build/post_office -run seq,new2 -range {rrr} -n {n} -cost {cost} &>>{log_dir}/{name}_par.txt'
            subprocess.call(cmd, shell=True)
            cmd = f'PARLAY_NUM_THREADS=1 ./build/post_office -run new2 -range {rrr} -n {n} -cost {cost} &>>{log_dir}/{name}_seq.txt'
            subprocess.call(cmd, shell=True)


if __name__ == '__main__':
    main()
