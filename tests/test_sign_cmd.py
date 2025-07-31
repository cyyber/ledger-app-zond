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

    transaction = bytes.fromhex("02f001038477359400850ba43b74008261a894b94f5374fce5edbc8e2a8697c15331677e6ebf0b88016345785d8a000080c0")

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
    assert response.data.hex() == "4dd68f397e1b07969434e915e80c4598b0d12438fda9667368e873eb2f8e5c64b52728707fffc5c0cbc7697b998e5f0f89ae5a67f4abd69401949842fa2b7653b24fc8963d7facdcf282932b6efb17b5eaf415f153ad796655db72a25c0d34eda95c99d03ca8c2de601ac1ed14b5e639290298f007e2f7e2c593f038059720d1b1d7b30b2a97a158270cedf92d38daae2bf1ec69af50cca73deee6e6bcec600e0b837e21e5e2e02f437d45b5c8d9511e091d0cb83055f06adfc02e4d11269942cbacde6a2c4af0345668ea38d30a0e627142cf17f404d84c4524222e6ad129a76be7d429d1f3cc72753c8f4fbe5bd627959afe961b645c03f9628f329bb8c6194a2e"

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

    transaction = bytes.fromhex("02f001038477359400850ba43b74008261a894b94f5374fce5edbc8e2a8697c15331677e6ebf0b88016345785d8a000080c0")

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_tx(path=path, transaction=transaction):
            scenario_navigator.review_reject()

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0
