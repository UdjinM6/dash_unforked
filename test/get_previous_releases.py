#!/usr/bin/env python3
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Download or build previous releases.
# Needs curl and tar to download a release, or the build dependencies when
# building a release.

import argparse
import contextlib
from fnmatch import fnmatch
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import hashlib


SHA256_SUMS = {
    "a4b555b47f5f9a5a01fc5d3b543731088bd10a65dd7fa81fb552818146e424b5":  "dashcore-19.3.0-aarch64-linux-gnu.tar.gz",
    "1b4673a2bd71f9f2b593c2d71386e60f4744b59b57142707f0045ed49c92024b":  "dashcore-19.3.0-osx64.tar.gz",
    "c2f3ff5631094abe16af8e476d1197be8685ee20601deda5cad0c34fc879c3de":  "dashcore-19.3.0-x86_64-linux-gnu.tar.gz",
    #
    "2870149fda49e731fdf67951408e8b9a1f21f4d91693add0287fe6abb7f8e5b4":  "dashcore-19.1.0-aarch64-linux-gnu.tar.gz",
    "900f6209831f1a21be7ed51edd48a6312fdfb8759fac94b77b23d77484254356":  "dashcore-19.1.0-osx64.tar.gz",
    "4aad6aedd3b45ae8c5279ad6ee886e7d80a1faa59be9bae882bdd6df68992990":  "dashcore-19.1.0-x86_64-linux-gnu.tar.gz",
    #
    "d7907726666e9266f5eae830789a1c36cf8c84b43bc0c0fab907317a5cc03f09":  "dashcore-18.2.2-aarch64-linux-gnu.tar.gz",
    "9b376e99400a3b0cb8e777477cf07567c36ed65018e4becbd98eebfbcca3efee":  "dashcore-18.2.2-osx64.tar.gz",
    "ebe3170835232c0a1e7456712fbb285d5749cbf3cfb5de29f8db3a2ad81be1cf":  "dashcore-18.2.2-x86_64-linux-gnu.tar.gz",
    #
    "57b7b9a7939a726afc133229dbecc4f46394bf57d32bf3e936403ade2b2a8519":  "dashcore-0.17.0.3-aarch64-linux-gnu.tar.gz",
    "349c65fb6c0d2d509d338b5bd8ca25b069b84c6d57b49a9d1bc830744e09926b":  "dashcore-0.17.0.3-osx64.tar.gz",
    "d4086b1271589e8d72e6ca151a1c8f12e4dc2878d60ec69532d0c48e99391996":  "dashcore-0.17.0.3-x86_64-linux-gnu.tar.gz",
    #
    "b0fd7b1344701f6b96f6b6978fbce7fd5d3e0310a2993e17858573d80e2941c0":  "dashcore-0.16.1.1-aarch64-linux-gnu.tar.gz",
    "3f26d7da7b3ea5ce1fabf34b4086a978324d5806481dc8470b15294a0807100d":  "dashcore-0.16.1.1-osx64.tar.gz",
    "8803928bd18e9515f1254f97751eb6e537a084d66111ce7968eafb23e26bf3a5":  "dashcore-0.16.1.1-x86_64-linux-gnu.tar.gz",
    #
    "e2c7f2566e26420a54c8d08e1f8a8d5595bb22fba46d3a84ab931f5cd0efc7f9":  "dashcore-0.15.0.0-aarch64-linux-gnu.tar.gz",
    "f5e5d25df3d84a9e5dceef43d0bcf54fa697ea73f3e29ec39a8f9952ace8792c":  "dashcore-0.15.0.0-osx64.tar.gz",
    "4cc0815ebd595f3d0134a8df9e6224cbe3d79398a5a899b60ca5f4ab8a576160":  "dashcore-0.15.0.0-x86_64-linux-gnu.tar.gz",
}


@contextlib.contextmanager
def pushd(new_dir) -> None:
    previous_dir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(previous_dir)


def download_binary(tag, args) -> int:
    if Path(tag).is_dir():
        if not args.remove_dir:
            print('Using cached {}'.format(tag))
            return 0
        shutil.rmtree(tag)
    Path(tag).mkdir()
    bin_path = 'releases/download/v{}'.format(tag[1:])
    match = re.compile('v(.*)(rc[0-9]+)$').search(tag)
    if match:
        bin_path = 'releases/download/test.{}'.format(
            match.group(1), match.group(2))
    tarball = 'dashcore-{tag}-{platform}.tar.gz'.format(
        tag=tag[1:], platform=args.platform)
    tarballUrl = 'https://github.com/dashpay/dash/{bin_path}/{tarball}'.format(
        bin_path=bin_path, tarball=tarball)

    print('Fetching: {tarballUrl}'.format(tarballUrl=tarballUrl))

    header, status = subprocess.Popen(
        ['curl', '--head', tarballUrl], stdout=subprocess.PIPE).communicate()
    if re.search("404 Not Found", header.decode("utf-8")):
        print("Binary tag was not found")
        return 1

    curlCmds = [
        ['curl', '-L', '--remote-name', tarballUrl]
    ]

    for cmd in curlCmds:
        ret = subprocess.run(cmd).returncode
        if ret:
            return ret

    hasher = hashlib.sha256()
    with open(tarball, "rb") as afile:
        hasher.update(afile.read())
    tarballHash = hasher.hexdigest()

    if tarballHash not in SHA256_SUMS or SHA256_SUMS[tarballHash] != tarball:
        if tarball in SHA256_SUMS.values():
            print("Checksum did not match")
            return 1

        print("Checksum for given version doesn't exist")
        return 1
    print("Checksum matched")

    # Extract tarball
    # special case for v17 and earlier: other name of version
    filename = tag[1:-2] if tag[1:3] == "0." else tag[1:]
    ret = subprocess.run(['tar', '-zxf', tarball, '-C', tag,
                          '--strip-components=1',
                          'dashcore-{tag}'.format(tag=filename, platform=args.platform)]).returncode
    if ret:
        return ret

    Path(tarball).unlink()
    return 0


def build_release(tag, args) -> int:
    githubUrl = "https://github.com/dashpay/dash"
    if args.remove_dir:
        if Path(tag).is_dir():
            shutil.rmtree(tag)
    if not Path(tag).is_dir():
        # fetch new tags
        subprocess.run(
            ["git", "fetch", githubUrl, "--tags"])
        output = subprocess.check_output(['git', 'tag', '-l', tag])
        if not output:
            print('Tag {} not found'.format(tag))
            return 1
    ret = subprocess.run([
        'git', 'clone', githubUrl, tag
    ]).returncode
    if ret:
        return ret
    with pushd(tag):
        ret = subprocess.run(['git', 'checkout', tag]).returncode
        if ret:
            return ret
        host = args.host
        if args.depends:
            with pushd('depends'):
                ret = subprocess.run(['make', 'NO_QT=1']).returncode
                if ret:
                    return ret
                host = os.environ.get(
                    'HOST', subprocess.check_output(['./config.guess']))
        config_flags = '--prefix={pwd}/depends/{host} '.format(
            pwd=os.getcwd(),
            host=host) + args.config_flags
        cmds = [
            './autogen.sh',
            './configure {}'.format(config_flags),
            'make',
        ]
        for cmd in cmds:
            ret = subprocess.run(cmd.split()).returncode
            if ret:
                return ret
        # Move binaries, so they're in the same place as in the
        # release download
        Path('bin').mkdir(exist_ok=True)
        files = ['dashd', 'dash-cli', 'dash-tx']
        for f in files:
            Path('src/'+f).rename('bin/'+f)
    return 0


def check_host(args) -> int:
    args.host = os.environ.get('HOST', subprocess.check_output(
        './depends/config.guess').decode())
    if args.download_binary:
        platforms = {
            'aarch64-*-linux*': 'aarch64-linux-gnu',
            'x86_64-*-linux*': 'x86_64-linux-gnu',
            'x86_64-apple-darwin*': 'osx64',
            'aarch64-apple-darwin*': 'osx64',
        }
        args.platform = ''
        for pattern, target in platforms.items():
            if fnmatch(args.host, pattern):
                args.platform = target
        if not args.platform:
            print('Not sure which binary to download for {}'.format(args.host))
            return 1
    return 0


def main(args) -> int:
    Path(args.target_dir).mkdir(exist_ok=True, parents=True)
    print("Releases directory: {}".format(args.target_dir))
    ret = check_host(args)
    if ret:
        return ret
    if args.download_binary:
        with pushd(args.target_dir):
            for tag in args.tags:
                ret = download_binary(tag, args)
                if ret:
                    return ret
        return 0
    args.config_flags = os.environ.get('CONFIG_FLAGS', '')
    args.config_flags += ' --without-gui --disable-tests --disable-bench'
    with pushd(args.target_dir):
        for tag in args.tags:
            ret = build_release(tag, args)
            if ret:
                return ret
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', '--remove-dir', action='store_true',
                        help='remove existing directory.')
    parser.add_argument('-d', '--depends', action='store_true',
                        help='use depends.')
    parser.add_argument('-b', '--download-binary', action='store_true',
                        help='download release binary.')
    parser.add_argument('-t', '--target-dir', action='store',
                        help='target directory.', default='releases')
    parser.add_argument('tags', nargs='+',
                        help="release tags. e.g.: v19.1.0 v19.0.0-rc.9")
    args = parser.parse_args()
    sys.exit(main(args))
