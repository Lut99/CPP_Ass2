import subprocess
import sys

AMP_POINTS = [10**3, 10**4, 10**5, 10**6, 10**7]
SYS_CONFIG = [(1, 8), (8, 1), (8, 8)]

def test(path, use_das, no_mpi):
    to_print = []
    for (num_nodes, num_cores) in SYS_CONFIG:
        for (i, points) in enumerate(AMP_POINTS):
            timesteps = AMP_POINTS[(len(AMP_POINTS) - 1) - i]
            # Run the subprocess
            to_run = ["mpirun", "-np", str(num_nodes * num_cores)]
            if no_mpi and use_das:
                to_run = ["prun", "-v", "-np", "1"]
            elif no_mpi:
                to_run = []
            elif use_das:
                to_run = ["prun", "-v", "-np", str(num_nodes), "-" + str(num_cores), "-sge-script", "$PRUN_ETC/prun-openmpi"]
            to_run += [path, str(points), str(timesteps)]
            p = subprocess.Popen(to_run, stdout=subprocess.PIPE)
            (out, _) = p.communicate()
            
            time_took = None
            time_took_normalized = None
            for line in out.decode("utf-8").split("\n"):
                words = line.split()
                if len(words) > 0:
                    if words[0] == "Took":
                        time_took = float(words[1])
                    elif words[0] == "Normalized:":
                        time_took_normalized = float(words[1])
            if time_took is None or time_took_normalized is None:
                to_print.append("{},{},{},-,-".format(num_nodes, num_cores, points))
            else:
                to_print.append("{},{},{},{},{}".format(num_nodes, num_cores, points, time_took, time_took_normalized))
    for s in to_print:
        print(s)


if __name__ == "__main__":
    import argparse

    p = argparse.ArgumentParser()
    p.add_argument("-p", "--path", help="The path to the file that is to be tested", default="./assign2_1/assign2_1")
    p.add_argument("--use_das", help="Uses the DAS arguments if given", action="store_true")
    p.add_argument("--no_mpi", help="If given, runs without mpi", action="store_true")

    args = p.parse_args()

    test(args.path, args.use_das, args.no_mpi)