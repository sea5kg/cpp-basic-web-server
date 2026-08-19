#!/usr/bin/env python3
##################################################################################
#
#        ▜ ▘▗ ▗ ▜      ▌      ▜   ▌
# ▛▛▌▌▌  ▐ ▌▜▘▜▘▐ █▌  ▛▌█▌▌▌  ▐ ▀▌▛▌
# ▌▌▌▙▌  ▐▖▌▐▖▐▖▐▖▙▖  ▙▌▙▖▚▘  ▐▖█▌▙▌
#    ▄▌
#
# MIT License
#
# Copyright (c) 2025-2026 Evgenii Sopov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Original repository: https://github.com/sea5kg/my-little-dev-lab
#
##################################################################################

""" Config for a pm """

import re
import os


class PmConfig:
    """ PmConfig """

    def __init__(self, root_dir):
        self.__root_dir = root_dir
        self.__re_uuid = re.compile(
            r'.*\"([0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12})\".*'
        )
        self.__base_tag = "sea5kg/my-little-dev-lab"
        self.__repo_url = "https://github.com/sea5kg/my-little-dev-lab"

    def get_root_dir(self):
        """ return root dir """
        return self.__root_dir

    def get_re_uuid(self):
        """ return regular expression for a search uuid in string """
        return self.__re_uuid

    def base_docker_tag(self):
        """ return docker base tag image """
        return self.__base_tag

    def repo_url(self):
        """ return repository link """
        return self.__repo_url


class FolderSwitcher:
    """
        Change work directory to specify folder
        And on exit change back work directory
    """
    def __init__(self, _log, new_dir):
        self.__prev = os.getcwd()
        self.__new_dir = new_dir
        self.__log = _log
        os.chdir(self.__new_dir)
        self.__log.debug(
            "FolderSwitcher (begin): %s -> %s", self.__prev, self.__new_dir
        )

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        os.chdir(self.__prev)
        self.__log.debug(
            "FolderSwitcher (end): %s -> %s", self.__new_dir, self.__prev
        )
