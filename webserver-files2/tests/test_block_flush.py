from signal import SIGINT

from time import sleep
import pytest
from requests import Session, exceptions
from server import Server, server_port
from definitions import DYNAMIC_OUTPUT_CONTENT, SERVER_CONNECTION_OUTPUT
from utils import spawn_clients, generate_dynamic_headers, validate_out, validate_response_full, validate_response_full_with_dispatch
from requests_futures.sessions import FuturesSession


@pytest.mark.parametrize("threads, queue, amount, dispatches",
                         [
                             (2,2,3,[0,0,4.8]),
                        ])
def test_sanity(threads, queue, amount, dispatches, server_port):
    with Server("./server", server_port, queue, queue, "bf") as server:
        with FuturesSession() as session1:
            session1.get(f"http://localhost:{server_port}/output.cgi?5")
            sleep(0.1)
            with FuturesSession() as session2:
                session2.get(f"http://localhost:{server_port}/output.cgi?1")
                sleep(0.1)
                with FuturesSession() as session3:
                    with pytest.raises(exceptions.ConnectionError):
                        resp = session3.get(f"http://localhost:{server_port}/output.cgi?1").result()
                        expected_headers = generate_dynamic_headers(123, 2, 0, 2)
                        expected = DYNAMIC_OUTPUT_CONTENT.format(seconds=f"1.{0:0<1}")
                        validate_response_full_with_dispatch(resp, expected_headers, expected, 4.8)

@pytest.mark.parametrize("threads, queue, spin_times, dispatches",
                         [
                             (2,2,[5,1,1,1],[0,0,-1,0]),
                             (2,2,[1,5,1,1],[0,0,-1,0]),
                             (1,1,[1,1],[0,-1]),
                             (2,2,[1,1,1],[0,0,-1]),
                             (2,4,[1,1,1,1,1],[0,0,0.8,0.8,-1])
                         ])
def test_load(threads, queue, spin_times, dispatches, server_port):
    with Server("./server", server_port, threads, queue, "bf") as server:
        clients = []
        for i in range(len(spin_times)):
            session = FuturesSession()
            clients.append((session, session.get(f"http://localhost:{server_port}/output.cgi?{spin_times[i]}")))
            sleep(0.1)

        for i in range(len(spin_times)):
            if dispatches[i] == -1:
                with pytest.raises(exceptions.ConnectionError):
                    clients[i][1].result()
            else:
                print(f"handling request {i}")
                resp = clients[i][1].result()
                clients[i][0].close()

                expected_headers = generate_dynamic_headers(123, (i // threads) + 1, 0, (i // threads) + 1)
                expected = DYNAMIC_OUTPUT_CONTENT.format(seconds=f"{spin_times[i]:.1f}")
                validate_response_full_with_dispatch(resp, expected_headers, expected, dispatches[i])