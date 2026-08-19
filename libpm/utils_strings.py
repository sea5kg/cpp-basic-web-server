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

""" Helpers for processing strings """


class UtilsStrings:
    """ UtilsStrings """

    @staticmethod
    def to_camelcase(snake_str):
        """
        to_camelcase

        from https://stackoverflow.com/users/487903/jbaiter
        """
        return "".join(x.capitalize() for x in snake_str.lower().split("_"))

    @staticmethod
    def snakecase_to_camelcase(snake_str):
        """
        We capitalize the first letter of each component except the first one
        with the 'capitalize' method and join them together.

        from https://stackoverflow.com/users/487903/jbaiter
        """

        camel_string = UtilsStrings.to_camelcase(snake_str)
        return snake_str[0].upper() + camel_string[1:]
