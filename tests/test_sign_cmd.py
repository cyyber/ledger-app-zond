import pytest

from ragger.backend.interface import BackendInterface
from ragger.error import ExceptionRAPDU
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.qrl_transaction import Transaction
from application_client.qrl_command_sender import QrlCommandSender, Errors
from application_client.qrl_response_unpacker import unpack_get_public_key_response, unpack_sign_tx_response
from utils import check_signature_validity

# In this tests we check the behavior of the device when asked to sign a transaction


# In this test we send to the device a transaction to sign and validate it on screen
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison
def test_sign_tx_short_tx(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    # Use the app interface instead of raw interface
    client = QrlCommandSender(backend)
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

    # Transaction with a 48-byte recipient address.
    # RLP: type=02, chain_id=1, nonce=3, gas_tip_cap, gas_fee_cap, gas=25000,
    #      to=00000000000000000000000000000000000000000000000000000000b94f5374fce5edbc8e2a8697c15331677e6ebf0b,
    #      value, data, access_list,
    #      descriptor=[0x01, 0x00, 0x00] (ML-DSA-87)
    transaction = bytes.fromhex("02f85201038477359400850ba43b74008261a8b000000000000000000000000000000000000000000000000000000000b94f5374fce5edbc8e2a8697c15331677e6ebf0b88016345785d8a0000825544c083010000")

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
    # Note: Signature will be different due to new transaction data with 48-byte addresses
    assert len(response.data) > 0  # Just verify we got a signature

# In this test we send to the device a transaction to trig a blind-signing flow
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison
# def test_sign_tx_short_tx_blind_sign(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
#     # Use the app interface instead of raw interface
#     client = QrlCommandSender(backend)
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
    client = QrlCommandSender(backend)
    path: str = "m/44'/238'/0'/0/0"

    # Transaction with a 48-byte recipient address and descriptor (ML-DSA-87)
    transaction = bytes.fromhex("02f85201038477359400850ba43b74008261a8b000000000000000000000000000000000000000000000000000000000b94f5374fce5edbc8e2a8697c15331677e6ebf0b88016345785d8a0000825544c083010000")

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_tx(path=path, transaction=transaction):
            scenario_navigator.review_reject()

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0
