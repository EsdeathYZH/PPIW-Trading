import multiprocessing
import pickle
import random
from enum import Enum

import threading
import time

# Fault Tolerant
# Master Process create a heart beat threads for each Slave Process
#   If timeout touched or Slave response nothing,
#       recreate a Slave Process to redo his job and kill old one.
# We should also keep a IPC channel for each M/S pair to do heartbeat

class Master:
    def __init__(self, file_list, words_dict, slave_id_list) -> None:
        self.file_list = file_list
        self.words_dict = words_dict
        self.slave_id_list = slave_id_list

        ### For fault tolerance
        self.heart_beat_threads = {} # {sid: heart_beat_thread}
        pass

    def run(self):
        ### TODO(for ft): start heartbeat tracking once Slaves started
        self.create_heart_beat()
        pass

    def create_heart_beat(self) -> None:
        for sid in self.slave_id_list:
            t = threading.Thread(target=self.ft_thread, args=(sid,))
            self.heart_beat_threads[sid] = t
            # TODO: once Slave done its work, Master should kill corresponding heart_beat_thread
            t.start()

    def ft_thread(self, sid) -> None:
        # Configure
        HEARTBEAT_TIMEOUT = 15 # seconds

        # TODO: create a IPC channel with Slave node with id=sid
        ipc_pipe = None

        lefttime = HEARTBEAT_TIMEOUT
        while lefttime > 0:
            time.sleep(1)
            lefttime -= 1
            try:
                r = ipc_send("HB", sid)
            except:
                self.do_retry(sid, '''TODO: WORK''')
                return
        self.do_retry(sid, '''TODO: WORK''')
        return

    def do_retry(self, sid):
        new_slave = Slave(sid, ErrorType(random.randint(0, 2)))
        new_slave_p = multiprocessing.Process(target=new_slave.run)
        new_slave_p.start()

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
