from time import sleep
import pytest

from server import Server, server_port
from definitions import DYNAMIC_OUTPUT_CONTENT, SERVER_CONNECTION_OUTPUT
from requests import Session, exceptions
from utils import spawn_clients, generate_dynamic_headers, validate_out, validate_response_full, validate_response_full_with_dispatch
from requests_futures.sessions import FuturesSession


@pytest.mark.parametrize("threads, queue, new_queue_size, spin_times, dispatches",
                         [
                             (1,1,5,[1,1,1,1],[0,-1,0.8,-1]),
                         ])
def test_sanity(threads, queue, new_queue_size, spin_times, dispatches, server_port):
    with Server("./server", server_port, queue, queue, "dynamic", new_queue_size=new_queue_size) as server:
        clients = []
        for i in range(len(spin_times)):
            session = FuturesSession()
            clients.append((session, session.get(f"http://localhost:{server_port}/output.cgi?{spin_times[i]}")))
            sleep(0.1)

        for i in range(len(spin_times)):
            if dispatches[i] == -1:
                with pytest.raises(exceptions.ConnectionError):
                    resp = clients[i][1].result()
                    clients[i][0].close()
            else:
                resp = clients[i][1].result()
                clients[i][0].close()
                assert abs(float(resp.headers['Stat-Req-Dispatch'][2:]) - dispatches[i]) < 0.1

@pytest.mark.parametrize("threads, queue, new_queue_size, spin_times, dispatches",
                         [
                             (1,1,4,[5,1,1,1,1,1,1,1],[0,-1,0.8,-1,0.8,-1,1,-1]),
                                   #   2   3   4   5
                             (2,4,6,[2,2,2,2,2],[0,0,1.8,1.7,-1]),
                                   #         |
                             (1,1,2,[5,1,1,1,1,1,1],[0,-1,1,-1,-1,-1,-1,-1,-1]),
                         ])
def test_load(threads, queue, new_queue_size, spin_times, dispatches, server_port):
    with Server("./server", server_port, queue, queue, "dynamic", new_queue_size=new_queue_size) as server:
        clients = []
        for i in range(len(spin_times)):
            session = FuturesSession()
            clients.append((session, session.get(f"http://localhost:{server_port}/output.cgi?{spin_times[i]}")))
            sleep(0.1)

        for i in range(len(spin_times)):
            if dispatches[i] == -1:
                with pytest.raises(exceptions.ConnectionError):
                    resp = clients[i][1].result()
                    clients[i][0].close()
            else:
                resp = clients[i][1].result()
                clients[i][0].close()