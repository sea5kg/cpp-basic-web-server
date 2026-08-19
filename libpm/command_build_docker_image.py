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

""" rebuild images """

import os
import sys
import logging
import yaml
from .utils_log import UtilsLog
from .utils_docker import UtilsDocker
from .pm_config import PmConfig


class CommandBuildDockerImage:
    """ CommandBuildDockerImage """

    def __init__(self, config: PmConfig):
        self.__log = UtilsLog("CommandBuildDockerImage").get_logger()
        self.__log.setLevel(logging.DEBUG)
        self.__config = config
        self.__subcommand_name = "build-docker-image"

    def get_name(self):
        """ return subcommand name """
        return self.__subcommand_name

    def do_registry(self, subparsers):
        """ registering sub command """
        desc = "Build docker image (Dockerfile)"
        _parser_build_image = subparsers.add_parser(
            name=self.__subcommand_name,
            description=desc
        )
        _parser_build_image.set_defaults(subparser=self.__subcommand_name)

    def execute(self, _):
        """ executing """
        os.chdir(self.__config.get_root_dir())
        self.__log.info("Build docker image...")
        wsjcpp_yaml_path = os.path.join(self.__config.get_root_dir(), "wsjcpp.yml")
        version = None
        with open(wsjcpp_yaml_path, 'r') as file:
            data = yaml.safe_load(file)
            version = data["version"]

        tag_build_latest = self.__config.base_docker_tag() + ":latest"
        tag_build_version = self.__config.base_docker_tag() + ":" + version

        UtilsDocker.silent_remove_image(tag_build_latest, self.__log)
        UtilsDocker.silent_remove_image(tag_build_version, self.__log)
        UtilsDocker.build_docker_image(tag_build_latest, "Dockerfile", self.__log)
        UtilsDocker.build_docker_image(tag_build_version, "Dockerfile", self.__log)

        sys.exit(0)
