import multiprocessing
import pickle
import random
from enum import Enum

class Master:
    def __init__(self, file_list, words_dict, slave_id_list) -> None:
        pass

    def run(self):
        pass

class ErrorType(Enum):
    LateStart = 0
    DontStart = 1
    StartbutCrash = 2
    # ....

class Slave:
    def __init__(self, my_id, control_arg: ErrorType) -> None:
        self.slave_id = my_id
        self.map_cnt = 0

    def run(self):
        # keep fetching task from master channel
        task_str = recv()
        task_infos = task_str.split()
        if task_infos[0] == "M":
            map_func(task_infos[1])
        else if task_infos[0] == "R":
            task_words = []
            for i in range(1, len(task_infos)):
                task_words.append(task_infos[i])
            reduce_func(task_words)
        else:
            print("Invalid task format!")
            assert(false)

    def map_func(self, input_path):
        # define in/out file
        input_file = open(input_path, "r")
        output_path = "/map_output/map_res_" + slave_id + "_" + map_cnt
        output_file = open(output_path, "w")

        # get all lines from file
        file_lines = input_file.readlines()
        for line in file_lines:
            # remove blankspace
            line = line.strip()

            # split file line
            words = line.split()

            # output tuples (word, 1)
            for word in words:
                tuple_str = word + " 1\n"
                output_file.write(tuple_str)

        input_file.close()
        output_file.close()
        # increase map_cnt
        map_cnt = map_cnt + 1

    def reduce_func(self, task_words):
        # define in/out file
        input_files = os.listdir("/map_output")
        output_path = "/reduce_output/reduce_res_" + slave_id
        output_file = open(output_path, "w")

        # final result
        word2count = {}

        for input_path in input_files:
            input_file = open("/map_output/" + input_path, "r")

            # get all lines from file
            file_lines = input_file.readlines()
            for line in file_lines:
                # remove blankspace
                line = line.strip()

                # parse the input from map func
                word, count = line.split(' ', 1)

                # omit words belong to other slaves
                if not word in task_words:
                    continue

                try:
                    count = int(count)
                except ValueError:
                    continue

                try:
                    word2count[word] = word2count[word]+count
                except:
                    word2count[word] = count

            # write the tuples to output reduce file
            for word in word2count.keys():
                output_file.write(word + " " + str(word2count[word] + "\n"))

            input_file.close()

        # close output
        output_file.close()


if __name__ == '__main__':
    pending_file_list = [f'file_{i}.txt' for i in range(10)]
    slave_id_list = [f'Slave{i}' for i in range(3)]
    with open('req_words.pkl', 'rb') as f:
        words_dict = pickle.load(f)  # {'Slave1': ['a', 'and'], 'Slave2': ['Alice', 'Bob']}

    # Create a Master
    m = Master(pending_file_list, words_dict, slave_id_list)
    # Create some slaves
    s_list = [Slave(slave_id, ErrorType(random.randint(0, 2))) for slave_id in slave_id_list]

    # run in multiprocess
    m_p = multiprocessing.Process(target=m.run)
    s_p_list = [multiprocessing.Process(target=s.run) for s in s_list]
    m_p.start()
    for p in s_p_list:
        p.start()
    m_p.join()
    for p in s_p_list:
        p.join()
