"""
https://stackoverflow.com/questions/6252833/mapping-basic-typedef-aliases-with-ctypes
typedefs aren't really their own symbols in the compiled code
"""
import ctypes
import glob
import base64
import json
import random
import os.path
import requests
import uuid
import pathlib
import lddwrap


class sss_shard(ctypes.Structure):
    _fields_ = [
        ("shard_num", ctypes.c_int),
        ("data_len", ctypes.c_int),
        ("mac", ctypes.POINTER(ctypes.c_uint8)),
        ("data", ctypes.POINTER(ctypes.c_uint8)),
    ]


class SSS(object):
    crypto_secretbox_KEYBYTES = 32  # 128 bits = 16 bytes
    crypto_secretbox_NONCEBYTES = 24
    SHARDS_TOTAL = 16
    SHARDS_REQUIRED = 8

    def __init__(self, sss_so_filepath, sodium_so_path):
        self._sodium_lib = ctypes.CDLL(sodium_so_path, mode=ctypes.RTLD_GLOBAL)
        self._sss_lib = ctypes.CDLL(sss_so_filepath, mode=ctypes.RTLD_GLOBAL)

        ########################################
        # Setup restype and argtypes and
        #  map functions

        self._randombytes = self._sodium_lib.randombytes
        self._randombytes.restype = ctypes.c_void_p
        self._randombytes.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),  # randombytes buffer
            ctypes.c_size_t,  # randombytes_len - the len
        ]

        self._randomize_file = self._sss_lib.randomize_file
        self._randomize_file.restype = ctypes.c_void_p
        self._randomize_file.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),  # input filepath
        ]

        self._unrandomize_file = self._sss_lib.unrandomize_file
        self._unrandomize_file.restype = ctypes.c_void_p
        self._unrandomize_file.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),  # input filepath
        ]


        self._encrypt_file = self._sss_lib.encrypt_file
        self._encrypt_file.restype = ctypes.c_void_p
        self._encrypt_file.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),  # input filepath
            ctypes.POINTER(ctypes.c_ubyte),  # output filepath
            ctypes.POINTER(ctypes.c_ubyte),  # key buffer
            ctypes.POINTER(ctypes.c_ubyte),  # nonce buffer
        ]

        self._decrypt_file = self._sss_lib.decrypt_file
        self._decrypt_file.restype = ctypes.c_void_p
        self._decrypt_file.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),  # input filepath
            ctypes.POINTER(ctypes.c_ubyte),  # output filepath
            ctypes.POINTER(ctypes.c_ubyte),  # key buffer
            ctypes.POINTER(ctypes.c_ubyte),  # nonce buffer
        ]

        self._shard_file = self._sss_lib.shard_file
        self._shard_file.restype = ctypes.c_void_p
        self._shard_file.argtypes = [
            ctypes.POINTER(ctypes.c_char),  # filename - the filename of the file to shard
            ctypes.POINTER(ctypes.c_char),  # shard prefix - the file prefix for saving shards
            ctypes.c_int,  # threshold number of shards required for reconstruction
            ctypes.c_int,  # total number of shards created
        ]

        self._unshard_file = self._sss_lib.unshard_file
        self._unshard_file.restype = ctypes.c_void_p
        self._unshard_file.argtypes = [
            ctypes.POINTER(ctypes.c_char),  # shard prefix - the file prefix for saving shards
            ctypes.POINTER(ctypes.c_char),  # outfile - the output filename
            ctypes.c_int,  # threshold number of shards required for reconstruction
            ctypes.c_int,  # total number of shards created
        ]

        ########################################
        self.processed = []

    def convert_string_to_c(self, s):
        return (ctypes.c_ubyte * (len(s) + 1))(*[ctypes.c_ubyte(ord(c)) for c in s])

    def randomize_file(self, filepath):

        c_filepath = self.convert_string_to_c(filepath)
        self._randomize_file(c_filepath)

    def unrandomize_file(self, filepath):

        c_filepath = self.convert_string_to_c(filepath)
        self._unrandomize_file(c_filepath)


    def encrypt_dependency(self, dep_filepath, enc_filepath, json_filepath):
        # print(f"DEBUG: encrypt_dependency( \
        # self, {dep_filepath}, {enc_filepath}, {json_filepath})")

        # create json and check if it is empty
        dependency_json = self.create_json(dep_filepath, os.path.dirname(enc_filepath))
        if not dependency_json:
            # if dependency_json is empty,
            # it was already processed or doesn't exist, so don't proceed.
            return

        # create key
        key_init_str = b"\x00" * self.crypto_secretbox_KEYBYTES
        c_key = (ctypes.c_ubyte * self.crypto_secretbox_KEYBYTES).from_buffer_copy(key_init_str)
        key = ctypes.cast(c_key, ctypes.c_char_p).value
        # get random key with no null bytes
        while (len(key) < self.crypto_secretbox_KEYBYTES) or (b"\x00" in key):
            self._randombytes(c_key, self.crypto_secretbox_KEYBYTES)
            # cast but limit to length=crypto_secretbox_KEYBYTES
            key = ctypes.cast(c_key, ctypes.c_char_p).value[: self.crypto_secretbox_KEYBYTES]

        # create nonce
        nonce_init_str = b"\x00" * self.crypto_secretbox_NONCEBYTES  # +1 for null byte
        c_nonce = (ctypes.c_ubyte * self.crypto_secretbox_NONCEBYTES).from_buffer_copy(
            nonce_init_str
        )
        nonce = ctypes.cast(c_nonce, ctypes.c_char_p).value

        # get random nonce with no null bytes
        while (len(nonce) < self.crypto_secretbox_NONCEBYTES) or (b"\x00" in nonce):
            self._randombytes(c_nonce, self.crypto_secretbox_NONCEBYTES)
            # cast but limit to length=crypto_secretbox_NONCEBYTES
            nonce = ctypes.cast(c_nonce, ctypes.c_char_p).value[: self.crypto_secretbox_NONCEBYTES]

        # print("key:", key)
        # print("len(key):", len(key))
        # print("nonce:", nonce)
        # print("len(nonce):", len(nonce))

        # cast filepaths to c
        c_dep_filepath = self.convert_string_to_c(dep_filepath)
        c_enc_filepath = self.convert_string_to_c(enc_filepath)
        # decrypted_dep_filepath = "{}.enc.dec".format(dep_filepath)
        # c_decrypted_dep_filepath = self.convert_string_to_c(decrypted_dep_filepath)

        # print(f"DEBUG: self._encrypt_file({dep_filepath}, {enc_filepath}, {key}, {nonce})")
        self._encrypt_file(c_dep_filepath, c_enc_filepath, c_key, c_nonce)
        # self._sss_decrypt_file(c_enc_filepath, c_decrypted_dep_filepath, c_key, c_nonce)
        self._randomize_file(c_enc_filepath)

        # insert key to json
        dependency_json["key"] = base64.b64encode(key).decode("utf-8")
        dependency_json["nonce"] = base64.b64encode(nonce).decode("utf-8")

        with open(json_filepath, "w", encoding="utf-8") as f:
            json.dump(dependency_json, f, indent=4)

    def decrypt_dependency(self, json_file, encrypted, decrypted):
        with open(json_file) as f:
            # don't use load because needed to strip null bytes
            j = json.load(f)

        key = j["key"]
        nonce = j["nonce"]

        print("key (b64'ed):", key)
        print("nonce (b64'ed):", nonce)

        key = base64.b64decode(j["key"].encode("utf-8"))
        nonce = base64.b64decode(j["nonce"].encode("utf-8"))
        print("key:", key)
        print("nonce:", nonce)

        c_key = (ctypes.c_ubyte * self.crypto_secretbox_KEYBYTES).from_buffer_copy(key)
        c_nonce = (ctypes.c_ubyte * self.crypto_secretbox_NONCEBYTES).from_buffer_copy(nonce)

        self._unrandomize_file(self.convert_string_to_c(encrypted))
        self._decrypt_file(
            self.convert_string_to_c(encrypted), self.convert_string_to_c(decrypted), c_key, c_nonce
        )

    def create_shards(self, input_filepath, shard_prefix):
        """
        input_filepath - file to shard
        shardprefix - keyshards file naming pattern prefix
        """
        input_filepath_c = ctypes.c_char_p(input_filepath.encode("utf-8"))
        shard_prefix_c = ctypes.c_char_p(shard_prefix.encode("utf-8"))

        self._shard_file(
            input_filepath_c, shard_prefix_c, self.SHARDS_REQUIRED, self.SHARDS_TOTAL
        )

    def combine_shards(self, shard_prefix, output_filepath):
        """
        shard_prefix - prefix for all shard files
        output_filepath - where to write decrypted file
        """
        shard_prefix_c = ctypes.c_char_p(shard_prefix.encode("utf-8"))
        output_filepath_c = ctypes.c_char_p(output_filepath.encode("utf-8"))

        self._unshard_file(
            shard_prefix_c, output_filepath_c, self.SHARDS_REQUIRED, self.SHARDS_TOTAL
        )

    def push_dependencies(self, name, registry, shard_host_list, shardprefix, encrypted_blob):
        shard_list = glob.glob("{}*".format(shardprefix))

        renamed_shards = []
        renamed_blobs = []

        # Rename & push shards
        for i in range(self.RS_SHARDS_TOTAL):
            # Rename shard
            shardname = shard_list.pop(0)
            new_shardname = str(uuid.uuid4())
            host = random.choice(shard_host_list)
            shard_url = f"https://{host}/{new_shardname}"
            renamed_shards.append(shard_url)

            # Push shard
            with open(shardname, "rb") as f:
                requests.post(shard_url, data=f.read(), verify=False)

        # Rename & push blob
        # loop TOTAL/NEEDED times to get same redundancy factor as shards have
        for i in range(int(self.SHARDS_TOTAL / self.SHARDS_REQUIRED)):
            # Rename blob
            new_blobname = str(uuid.uuid4())
            host = random.choice(shard_host_list)
            blob_url = f"https://{host}/{new_blobname}"
            renamed_blobs.append(blob_url)

            # Push blob
            with open(encrypted_blob, "rb") as f:
                requests.post(blob_url, data=f.read(), verify=False)

        # Send JSON to registry
        dep_json = {
            "name": name,
            "so_sharded": False,
            "so": renamed_blobs,
            "json_sharded": True,
            "json": renamed_shards,
        }

        registry_url = f"https://{registry}/{new_blobname}"
        requests.post(registry_url, json=dep_json, verify=False)

    def create_json(self, so_path, foldername):
        # print(f"DEBUG: create_json({so_path}, {foldername}")

        so_path = str(so_path)
        so_path = os.path.realpath(so_path)
        if not os.path.isfile(so_path):
            return
        if so_path in self.processed:
            return
        else:
            self.processed.append(so_path)

        so_filename = os.path.basename(so_path)
        # copy(so_path, so_filename)

        path = pathlib.Path(so_path)
        try:
            deps = lddwrap.list_dependencies(path=path)
        except RuntimeError:
            deps = []

        dependency_json = {}
        dependency_json["name"] = so_filename
        dependency_json["depends_on"] = []
        dependency_json["conflicts_with"] = []
        dependency_json["arch"] = "x86_64"
        # dependency_json["arch"] = "arm"

        std_libs = ["libpthread", "libc", "libdl", "ld", "linux-vdso"]

        for dep in deps:
            if dep.found and dep.path:
                # if dependency is found, the full path is in dep.path
                so_name = str(os.path.basename(dep.path))
            else:
                # if dependency is not found, the soname is in dep.soname
                so_name = dep.soname
                # print warning if not a standard library
                if not any([so_name.startswith(lib) for lib in std_libs]):
                    print(f"Warning: dependency not found locally: {so_name}.")
                    print("Be sure the dependency is available elsewhere.")
                continue

            # don't shard standard libraries
            if any([so_name.startswith(lib) for lib in std_libs]):
                continue

            so_full_path = os.path.join(os.path.dirname(dep.path), so_name)
            dep_enc_path = f"{foldername}/{so_name}.enc"
            dep_json_path = f"{foldername}/{so_name}.json"

            # encrypt subdependency
            self.encrypt_dependency(so_full_path, dep_enc_path, dep_json_path)
            # shard subdependency json
            self.create_shards(dep_json_path, f"{dep_json_path}.")

            # assert(os.path.isfile(so_full_path))
            if os.path.isfile(dep.path):
                dependency_json["depends_on"].append(so_name)

        return dependency_json

if __name__ == "__main__":
    # test()
    pass
    # https://stackoverflow.com/questions/4213095/python-and-ctypes-how-to-correctly-pass-pointer-to-pointer-into-dll
