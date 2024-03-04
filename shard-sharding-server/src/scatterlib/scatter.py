#!/usr/bin/env python3

import os
import sys
import logging
import argparse

from sss import sss

# -------------------------------------------------------------------------------
api = None
logger = None
# -------------------------------------------------------------------------------


# -------------------------------------------------------------------------------
# Utils
def load_api() -> object:
    sodium_so_path = os.getenv(
        "LIBSODIUM", default=None
    )  # /usr/lib/x86_64-linux-gnu/libsodium.so.23
    if not sodium_so_path:
        logger.error("'LIBSODIUM' environment variable must be specified")
        return None

    sss_so_path = os.getenv("LIBSSS", default=None)  # /usr/lib/x86_64-linux-gnu/libsss.so
    if not sodium_so_path:
        logger.error("'LIBSSS' environment variable must be specified")
        return None

    api = sss.SSS(sss_so_path, sodium_so_path)
    return api


# -------------------------------------------------------------------------------
# CLI Functions
# def foo(args):
#     utility(args.helloarg, args.fooarg)


def randomize(args):
    api.randomize_file(args.file)


def unrandomize(args):
    api.unrandomize_file(args.file)


def reconstruct_and_decrypt(args):
    api.combine_shards(args.shardprefix, args.json)
    api.decrypt_dependency(args.json, args.encrypted, args.decrypted)


def encrypt_and_shard(args):
    api.encrypt_dependency(args.file, args.encrypted, args.json)
    api.create_shards(args.json, args.shardprefix)


def shard(args):
    api.create_shards(args.file, args.shardprefix)


def reconstruct(args):
    api.combine_shards(args.shardprefix, args.reconstructed)


def decrypt(args):
    api.decrypt_dependency(args.json, args.encrypted, args.decrypted)


def encrypt(args):
    api.encrypt_dependency(args.file, args.encrypted, args.json)


def push_dependencies(args):
    api.push_dependencies(args.file, args.registry, args.hosts, args.shardprefix, args.encrypted)


# -------------------------------------------------------------------------------
if __name__ == "__main__":
    # -------------------------------------------------------------------------------
    # Base parser
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", "-v", action="store_true")
    subparser = parser.add_subparsers()

    shard_parser = subparser.add_parser("shard")
    shard_parser.add_argument("file", help="file to shard")
    shard_parser.add_argument(
        "--shardprefix", "-s", help="prefix to the shard files", default="default"
    )
    shard_parser.set_defaults(func=shard)

    reconstruct_parser = subparser.add_parser("reconstruct")
    reconstruct_parser.add_argument("shardprefix", help="prefix of the existing shard files")
    reconstruct_parser.add_argument(
        "--reconstructed", "-r", help="file to write the reconstructed data to", default="default"
    )
    reconstruct_parser.set_defaults(func=reconstruct)

    randomize_parser = subparser.add_parser("randomize")
    randomize_parser.add_argument("file", help="file to randomize")
    randomize_parser.set_defaults(func=randomize)

    unrandomize_parser = subparser.add_parser("unrandomize")
    unrandomize_parser.add_argument("file", help="file to unrandomize")
    unrandomize_parser.set_defaults(func=unrandomize)

    encrypt_parser = subparser.add_parser("encrypt")
    encrypt_parser.add_argument("file", help="file to encrypt")
    encrypt_parser.add_argument(
        "--encrypted", "-e", help="file to write the encrypted data to", default="default"
    )
    encrypt_parser.add_argument("--json", "-j", help="file to write the json", default="default")
    encrypt_parser.set_defaults(func=encrypt)

    decrypt_parser = subparser.add_parser("decrypt")
    decrypt_parser.add_argument("encrypted", help="encrypted file to decrypt")
    decrypt_parser.add_argument(
        "--decrypted", "-d", help="file to write the decrypted data to", default="default"
    )
    decrypt_parser.add_argument("--json", "-j", help="file to write the json", default="default")
    decrypt_parser.set_defaults(func=decrypt)

    push_dependencies_parser = subparser.add_parser("push_dependencies")
    push_dependencies_parser.add_argument("file", help="dependency filename")
    push_dependencies_parser.add_argument(
        "--shardprefix", "-s", help="prefix of the shard files", default="default"
    )
    push_dependencies_parser.add_argument(
        "--encrypted", "-e", help="file holding the encrypted data", default="default"
    )
    push_dependencies_parser.add_argument(
        "--registry", "-r", help="registry holding location of blobs", default="10.138.85.72:8080"
    )
    push_dependencies_parser.add_argument(
        "--hosts", help="hosts holding blobs", nargs="*", default=["10.138.85.72:4443"]
    )
    push_dependencies_parser.set_defaults(func=push_dependencies)

    encrypt_and_shard_parser = subparser.add_parser("encrypt_and_shard")
    encrypt_and_shard_parser.add_argument("file", help="file to encrypt_and_shard")
    encrypt_and_shard_parser.add_argument(
        "--shardprefix", "-s", help="prefix to name the shard files", default="default"
    )
    encrypt_and_shard_parser.add_argument(
        "--encrypted", "-e", help="file to write the encrypted data to", default="default"
    )
    encrypt_and_shard_parser.add_argument(
        "--json", "-j", help="file to write the json", default="default"
    )
    encrypt_and_shard_parser.add_argument(
        "--rs-blob", help="blob to write rs shard file to", default="default"
    )
    encrypt_and_shard_parser.set_defaults(func=encrypt_and_shard)

    reconstruct_and_decrypt_parser = subparser.add_parser("reconstruct_and_decrypt")
    reconstruct_and_decrypt_parser.add_argument(
        "--decrypted", "-d", help="file to reconstruct_and_decrypt", default="default"
    )
    reconstruct_and_decrypt_parser.add_argument("shardprefix", help="prefix to the shard files")
    reconstruct_and_decrypt_parser.add_argument("encrypted", help="file to decrypt")
    reconstruct_and_decrypt_parser.add_argument(
        "--json", "-j", help="file to write the json", default="default"
    )
    reconstruct_and_decrypt_parser.add_argument(
        "--rs-blob", help="blob to write rs shard file to", default="default"
    )
    reconstruct_and_decrypt_parser.set_defaults(func=reconstruct_and_decrypt)

    # -------------------------------------------------------------------------------
    # Parent parser for foo/bar, with parent argument
    # parent_parser = argparse.ArgumentParser(add_help=False)
    # parent_parser.add_argument('--parent', type=int)
    # -------------------------------------------------------------------------------

    # -------------------------------------------------------------------------------
    # Children parsers inheriting from parent_parser
    # foo_parser = subparser.add_parser('foo', help='foo help', parent_parsers=[parent_parser])
    # foo_parser.add_argument('--fooarg', default='foodef')
    # foo_parser.set_defaults(func=foo)
    #
    # bar_parser = subparser.add_parser('bar', help='bar help', parent_parsers=[parent_parser])
    # bar_parser.add_argument('--bararg', '-b', action='store_true')
    # bar_parser.set_defaults(func=bar)
    # #-------------------------------------------------------------------------------

    args = parser.parse_args()

    if hasattr(args, "shardprefix") and hasattr(args, "json") and args.shardprefix == "default":
        args.shardprefix = f"{args.file}.json."
    elif hasattr(args, "shardprefix") and args.shardprefix == "default":
        args.shardprefix = f"{args.file}."

    if hasattr(args, "encrypted") and args.encrypted == "default":
        args.encrypted = f"{args.file}.enc"

    if hasattr(args, "decrypted") and hasattr(args, "file") and args.decrypted == "default":
        args.decrypted = f"{args.file}.dec"
    elif hasattr(args, "decrypted") and hasattr(args, "encrypted") and args.decrypted == "default":
        args.decrypted = f"{args.encrypted}.dec"

    if hasattr(args, "file") and args.file == "default":
        args.file = "{}.dec".format(args.encrypted.split(".enc")[0])

    if hasattr(args, "json") and hasattr(args, "file") and args.json == "default":
        args.json = f"{args.file}.json"
    elif hasattr(args, "json") and hasattr(args, "encrypted") and args.json == "default":
        args.json = "{}.json".format(args.encrypted.split(".enc")[0])

    if (
        hasattr(args, "reconstructed")
        and hasattr(args, "shardprefix")
        and args.reconstructed == "default"
    ):
        args.reconstructed = f"{args.shardprefix}reconstructed"
    elif (
        hasattr(args, "reconstructed") and hasattr(args, "file") and args.reconstructed == "default"
    ):
        args.reconstructed = "{}.reconstructed".format(args.file.split(".rsblob")[0])

    if hasattr(args, "blob") and args.blob == "default":
        args.blob = f"{args.file}.blob"

    if hasattr(args, "rs_blob") and args.rs_blob == "default":
        if hasattr(args, "json"):
            args.rs_blob = f"{args.json}.rsblob"
        elif hasattr(args, "file"):
            args.rs_blob = f"{args.file}.rsblob"
        elif hasattr(args, "reconstructed"):
            args.rs_blob = "{}.rsblob".format(args.reconstructed.split(".reconstructed")[0])

    log_level = logging.INFO
    if args.verbose:
        log_level = logging.DEBUG
    logging.basicConfig(level=log_level, format="[*] %(message)s")
    logger = logging.getLogger(__name__)

    # validate environment and load api
    api = load_api()
    if not api:
        sys.exit()

    # ---------------------------------------
    if hasattr(args, "func"):
        args.func(args)
    else:
        parser.print_help()
    # ---------------------------------------
    # sys.exit()
