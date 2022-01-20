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
        pass

    def run(self):
        pass

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
