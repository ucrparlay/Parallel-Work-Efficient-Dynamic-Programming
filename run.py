
import subprocess


def main():
    n = 100000000
    rrr = 10000000000
    costs = [0]
    for t in range(20):
        costs.append(10**t)
    for cost in costs:
        cmd = f"./build/post_office -range {rrr} -n {n} -cost {cost}"
        subprocess.call(cmd, shell=True)


if __name__ == '__main__':
    main()
