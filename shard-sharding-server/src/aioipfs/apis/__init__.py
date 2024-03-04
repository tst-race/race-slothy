import json
import asyncio
import sys

from aiohttp.web_exceptions import (
    HTTPError,
    HTTPInternalServerError,
    HTTPServerError,
    HTTPBadRequest,
)
from aiohttp.client_exceptions import *  # noqa

from aioipfs.helpers import *  # noqa
from aioipfs.exceptions import *  # noqa


DEFAULT_TIMEOUT = 60 * 60

HTTP_ERROR_CODES = [
    HTTPInternalServerError.status_code,
    HTTPBadRequest.status_code,
    HTTPError.status_code,
    HTTPServerError.status_code,
]


class SubAPI(object):
    """
    Master class for all classes implementing API functions

    :param driver: the AsyncIPFS instance
    """

    def __init__(self, driver):
        self.driver = driver

    def url(self, path):
        return self.driver.api_endpoint(path)

    def decode_error(self, errormsg):
        try:
            decoded_json = json.loads(errormsg)
            msg = decoded_json["Message"]
            code = decoded_json["Code"]
            return msg, code
        except Exception:
            return None, None

    async def fetch_text(self, url, params={}, timeout=DEFAULT_TIMEOUT):
        async with self.driver.session.post(url, params=params) as response:
            status, textdata = response.status, await response.text()
            if status in HTTP_ERROR_CODES:
                msg, code = self.decode_error(textdata)
                raise APIError(code=code, message=msg, http_status=status)
            return textdata

    async def fetch_raw(self, url, params={}, timeout=DEFAULT_TIMEOUT):
        async with self.driver.session.post(url, params=params) as response:
            status, data = response.status, await response.read()
            if status in HTTP_ERROR_CODES:
                msg, code = self.decode_error(data)
                raise APIError(code=code, message=msg, http_status=status)
            return data

    async def fetch_json(self, url, params={}, timeout=DEFAULT_TIMEOUT):
        return await self.post(url, params=params, outformat="json")

    async def post(self, url, data=None, headers={}, params={}, outformat="text"):
        try:
            async with self.driver.session.post(
                url, data=data, headers=headers, params=params
            ) as response:
                if response.status in HTTP_ERROR_CODES:
                    msg, code = self.decode_error(await response.read())

                    if msg and code:
                        raise APIError(code=code, message=msg, http_status=response.status)
                    else:
                        raise UnknownAPIError()

                if outformat == "text":
                    return await response.text()
                elif outformat == "json":
                    return await response.json()
                elif outformat == "raw":
                    return await response.read()
                else:
                    raise Exception(f"Unknown output format {outformat}")
        except (APIError, UnknownAPIError) as apierr:
            if self.driver.debug:
                print(f"{url}: Post API error: {apierr}", file=sys.stderr)

            raise apierr
        except (
            ClientPayloadError,
            ClientConnectorError,
            ServerDisconnectedError,
            BaseException,
        ) as err:
            if self.driver.debug:
                print(f"{url}: aiohttp error: {err}", file=sys.stderr)

            return None

    async def mjson_decode(
        self,
        url,
        method="post",
        data=None,
        params=None,
        headers=None,
        new_session=False,
        timeout=60.0 * 60,
        read_timeout=60.0 * 10,
    ):
        """
        Multiple JSON objects response decoder (async generator), used for
        the API endpoints which return multiple JSON messages

        :param str method: http method, get or post
        :param data: data, for POST only
        :param params: http params
        """

        kwargs = {"params": params if params else {}}

        if new_session is True:
            session = self.driver.get_session(conn_timeout=timeout, read_timeout=read_timeout)
        else:
            session = self.driver.session

        if method not in ["get", "post"]:
            raise ValueError("mjson_decode: unknown method")

        if method == "post":
            if data is not None:
                kwargs["data"] = data

        if isinstance(headers, dict):
            kwargs["headers"] = headers

        try:
            async with getattr(session, method)(url, **kwargs) as response:
                async for raw_message in response.content:
                    message = decode_json(raw_message)

                    if message is not None:
                        if "Message" in message and "Code" in message:
                            raise APIError(
                                code=message["Code"],
                                message=message["Message"],
                                http_status=response.status,
                            )
                        else:
                            yield message

                    await asyncio.sleep(0)
        except asyncio.CancelledError as err:
            if new_session is True:
                await session.close()

            raise err
        except APIError as e:
            raise e
        except (
            ClientPayloadError,
            ClientConnectorError,
            ServerDisconnectedError,
            Exception,
        ):
            raise IPFSConnectionError("Connection/payload error")

        if new_session is True:
            await session.close()
