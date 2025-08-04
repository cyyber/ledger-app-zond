import pytest

from ragger.backend.interface import BackendInterface
from ragger.error import ExceptionRAPDU
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.boilerplate_transaction import Transaction
from application_client.boilerplate_command_sender import BoilerplateCommandSender, Errors
from application_client.boilerplate_response_unpacker import unpack_get_public_key_response, unpack_sign_tx_response
from utils import check_signature_validity

# In this tests we check the behavior of the device when asked to sign a transaction


# In this test we send to the device a transaction to sign and validate it on screen
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_tx_short_tx(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    # Use the app interface instead of raw interface
    client = BoilerplateCommandSender(backend)
    # The path used for this entire test
    path: str = "m/44'/238'/0'/0/0"

    # First we need to get the public key of the device in order to build the transaction
    # rapdu = client.get_public_key(path=path)
    # _, public_key, _, _ = unpack_get_public_key_response(rapdu.data)

    # Create the transaction that will be sent to the device for signing
    # transaction = Transaction(
    #     nonce=1,
    #     to="0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
    #     value=666,
    #     memo="For u EthDev"
    # ).serialize()

    transaction = bytes.fromhex("02f89601038477359400850ba43b74008261a898b94f5374fce5edbc8e2a8697c15331677e6ebf0b0000000088016345785d8a0000825544f85ff85d98b94f5374fce5edbc8e2a8697c15331677e6ebf0b00000000f842a00000000000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000000000000000000000000000000000000001")

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_tx(path=path, transaction=transaction):
        # Validate the on-screen request by performing the navigation appropriate for this device
        scenario_navigator.review_approve()

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response()
    # _, der_sig, _ = unpack_sign_tx_response(response)
    # assert check_signature_validity(public_key, der_sig, transaction)
    assert response.data.hex() == "c3172f4762fbda664a209b347ebe73d73f7bb6abef0c08e5f5721b7bcd2999a799177bfe0c729f3cd825dd068cd9dc2f9dfe3cd6c381fb44fd8a2fb73f640628ef2db66d8ab1071ece1d15b90999bb7fbfbeb9071eb45459abf51d298c96c4e53ff23b0ca87725887d78900425ee5c7481e278ab708a26a9b726e90b631ed46d16f98ed5d61e223a936b82c82df0e2b0eec1cf4bb6ffc9d225a8effcf92109afeb5ec182785d724132c58107ac5436b68287ea7112b716ae803a87f77ee863664c058fd31ddc70c9e91715d28f94d7b756a23741bae1db9ee2f99d24b0b4c2e639609a18cff1627ca9d01dd2b5a1be41b682425264ed108d17829dba7106eb0f676d"

# In this test we send to the device a transaction to trig a blind-signing flow
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison
# def test_sign_tx_short_tx_blind_sign(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
#     # Use the app interface instead of raw interface
#     client = BoilerplateCommandSender(backend)
#     # The path used for this entire test
#     path: str = "m/44'/238'/0'/0/0"

#     # Create the transaction that will be sent to the device for signing
#     # transaction = Transaction(
#     #     nonce=1,
#     #     to="0x0000000000000000000000000000000000000000",
#     #     value=0,
#     #     memo="Blind-sign"
#     # ).serialize()
#     transaction = bytes.fromhex("aabbccddee")

#     # As it requires on-screen validation, the function is asynchronous.
#     # It will yield the result when the navigation is done
#     with client.sign_tx(path=path, transaction=transaction):
#         # Validate the on-screen request by performing the navigation appropriate for this device
#         scenario_navigator.review_approve_with_warning(warning_path="part1")

#     # The device as yielded the result, parse it and ensure that the signature is correct
#     response = client.get_async_response().data
#     print(response.hex())
#     # _, der_sig, _ = unpack_sign_tx_response(response)
#     # assert check_signature_validity(public_key, der_sig, transaction)
#     assert True

# Transaction signature refused test
# The test will ask for a transaction signature that will be refused on screen
def test_sign_tx_refused(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    # Use the app interface instead of raw interface
    client = BoilerplateCommandSender(backend)
    path: str = "m/44'/238'/0'/0/0"

    transaction = bytes.fromhex("02f89601038477359400850ba43b74008261a898b94f5374fce5edbc8e2a8697c15331677e6ebf0b0000000088016345785d8a0000825544f85ff85d98b94f5374fce5edbc8e2a8697c15331677e6ebf0b00000000f842a00000000000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000000000000000000000000000000000000001")

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_tx(path=path, transaction=transaction):
            scenario_navigator.review_reject()

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0
