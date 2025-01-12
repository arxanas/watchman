# vim:ts=4:sw=4:et:
# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# no unicode literals
from __future__ import absolute_import, division, print_function

import json
import os
import os.path

import WatchmanTestCase


@WatchmanTestCase.expand_matrix
class TestFind(WatchmanTestCase.WatchmanTestCase):
    def test_find(self):
        root = self.mkdtemp()
        self.touchRelative(root, "foo.c")
        self.touchRelative(root, "bar.txt")

        self.watchmanCommand("watch", root)
        self.assertFileList(root, ["foo.c", "bar.txt"])

        # Make sure we correctly observe deletions
        os.unlink(os.path.join(root, "bar.txt"))
        self.assertFileList(root, ["foo.c"])

        # touch -> delete -> touch, should show up as exists
        self.touchRelative(root, "bar.txt")
        self.assertFileList(root, ["foo.c", "bar.txt"])
        os.unlink(os.path.join(root, "bar.txt"))

        # A moderately more complex set of changes
        os.mkdir(os.path.join(root, "adir"))
        os.mkdir(os.path.join(root, "adir", "subdir"))
        self.touchRelative(root, "adir", "subdir", "file")
        os.rename(
            os.path.join(root, "adir", "subdir"), os.path.join(root, "adir", "overhere")
        )

        self.assertFileList(
            root, ["adir", "adir/overhere", "adir/overhere/file", "foo.c"]
        )

        os.rename(os.path.join(root, "adir"), os.path.join(root, "bdir"))

        self.assertFileList(
            root, ["bdir", "bdir/overhere", "bdir/overhere/file", "foo.c"]
        )

        self.assertTrue(self.rootIsWatched(root))

        self.watchmanCommand("watch-del", root)
        self.waitFor(lambda: not self.rootIsWatched(root))

        self.assertFalse(self.rootIsWatched(root))
