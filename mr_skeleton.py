import multiprocessing
import pickle
import random
from enum import Enum

import threading
import time
import socket
import _thread
import os

# Fault Tolerant
# Master Process create a heart beat threads for each Slave Process
#   If timeout touched or Slave response nothing,
#       recreate a Slave Process to redo his job and kill old one.
# We should also keep a IPC channel for each M/S pair to do heartbeat

class Adaptor:
    def __init__(self, num_peer: int) -> None:
        self.send_sock = {}
        self.recv_sock = {}
        self.num_peer = num_peer

        for i in range(num_peer):
            for j in range(num_peer):
                socket.setblocking(0)
                sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                sockfile = "/tmp/{}-{}.sock".format(i,j)
                if os.path.exists(sockfile):
                    os.unlink(sockfile)
                sock.bind(sockfile)
                sock.listen(0)
                _thread.start_new_thread(sock.accept())

                if i not in self.send_sock:
                    self.send_sock[i] = []
                if j not in self.recv_sock:
                    self.recv_sock[j] = []
                self.send_sock[i].append(sock)
                self.recv_sock[j].append(sock)
        
        for i in range(num_peer):
            for j in range(num_peer):
                sockfile = "/tmp/{}-{}.sock".format(j,i)
                _thread.start_new_thread(self.send_sock[i].connect, (sockfile))


    def send(self, src: int, dst: int, msg: str) -> int:
        return self.send_sock[src][dst].send(msg)

    def recv(self, src: int) -> bytes:
        ret = None
        for sock in self.recv_sock[src]:
            ret = sock.recv()
            if len(ret) > 0:
                break
        return ret


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

class SlaveStateType(Enum):
    RUNNING = 0
    IDLE = 1
    ERROR = 2

class Slave:
    def __init__(self, my_id, control_arg: ErrorType) -> None:
        self.slave_id = my_id
        self.map_cnt = 0
        self.state = SlaveStateType.IDLE

    def run(self):
        # keep fetching task from master channel
        while(True):
            task_str = recv()
            state = SlaveStateType.RUNNING
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
            state = SlaveStateType.IDLE

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
