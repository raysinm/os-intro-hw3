from asyncio.subprocess import PIPE
from multiprocessing.dummy import current_process
from posixpath import basename
from subprocess import Popen, PIPE
import sys
import os
import pytest


class Server:
    def __init__(self, path, port, threads, queue_size, policy, should_pipe=True, new_queue_size=None):
        self.path = str(path)
        self.port = str(port)
        self.threads = str(threads)
        self.queue_size = str(queue_size)
        self.policy = str(policy)
        self.should_pipe = should_pipe
        self.new_queue_size = str(new_queue_size)

    def __enter__(self):
        args = [self.path, self.port, self.threads, self.queue_size, self.policy]
        if self.new_queue_size is not None:
            args.append(self.new_queue_size)
        self.process = Popen(args,
                             stdout=PIPE if self.should_pipe else False,
                             stderr=PIPE if self.should_pipe else False,
                             cwd="..",
                             bufsize=-1,
                             encoding=sys.getdefaultencoding())
        return self.process

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.process.terminate()

@pytest.fixture
def server_port(request):
    return 8000 + (abs(hash(basename(request.node.fspath) + request.node.name)) % 20000)
