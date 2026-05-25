from ragger.backend.interface import BackendInterface
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.qrl_command_sender import QrlCommandSender
from application_client.qrl_response_unpacker import unpack_get_public_key_response


def test_get_public_key_no_confirm(backend: BackendInterface) -> None:
    client = QrlCommandSender(backend)
    response = client.get_public_key(path="m/44'/238'/0'/0/0").data
    prefix, address = unpack_get_public_key_response(response)

    assert prefix == ord('Q')
    assert len(address) == 64


# In this test we check that the GET_PUBLIC_KEY works in confirmation mode
def test_get_public_key_confirm_accepted(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    client = QrlCommandSender(backend)
    path = "m/44'/238'/0'/0/0"
    with client.get_public_key_with_confirmation(path=path):
        scenario_navigator.address_review_approve()

    response = client.get_async_response().data
    prefix, address = unpack_get_public_key_response(response)

    assert prefix == ord('Q')
    assert len(address) == 64
