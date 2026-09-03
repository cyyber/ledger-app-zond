"""End-to-end helper: sign an arbitrary preimage on Speculos and dump the
address, public key, and signature for on-chain broadcast.

Gated behind E2E_PREIMAGE so normal CI runs skip it:

    E2E_PREIMAGE=<hex> E2E_OUT=/app/tests/e2e_out.json pytest \
        test_e2e_device_sign.py --device nanosp
"""
import json
import os

import pytest
from ragger.backend.interface import BackendInterface
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.boilerplate_command_sender import (
    BoilerplateCommandSender, CLA, InsType)

PK_LEN = 2592
SIG_LEN = 4627
PATH = "m/44'/238'/0'/0/0"


@pytest.mark.skipif(not os.environ.get("E2E_PREIMAGE"),
                    reason="e2e device signing runs only with E2E_PREIMAGE set")
def test_e2e_device_sign(backend: BackendInterface,
                         scenario_navigator: NavigateWithScenario) -> None:
    preimage = bytes.fromhex(os.environ["E2E_PREIMAGE"])
    out_path = os.environ.get("E2E_OUT", "/tmp/e2e_out.json")
    client = BoilerplateCommandSender(backend)

    # Address (no on-screen confirmation) - derives the key into NVM too.
    rapdu = client.get_public_key(path=PATH)
    assert rapdu.data[0:1] == b"Q" and len(rapdu.data) == 65
    address = rapdu.data[1:].hex()

    # Public key, chunked via P2=1..11.
    pk = b""
    for p2 in range(1, 12):
        r = backend.exchange(cla=CLA, ins=InsType.GET_PUBLIC_KEY, p1=0, p2=p2)
        pk += r.data
    pk = pk[:PK_LEN]
    assert len(pk) == PK_LEN

    # Sign the preimage; approve on screen without golden comparisons.
    with client.sign_tx(path=PATH, transaction=preimage):
        scenario_navigator.review_approve(do_comparison=False)
    sig = bytearray(client.get_async_response().data)

    # Remaining signature chunks via P1=2, P2=1..17.
    for p2 in range(1, 18):
        r = backend.exchange(cla=CLA, ins=InsType.SIGN_TX, p1=2, p2=p2)
        sig += r.data
    sig = bytes(sig)[:SIG_LEN]
    assert len(sig) == SIG_LEN

    with open(out_path, "w") as f:
        json.dump({"address": address, "public_key": pk.hex(),
                   "signature": sig.hex()}, f)
