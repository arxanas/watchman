/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var assert = require('assert');
var watchman = require('fb-watchman');
var client = new watchman.Client();
const fs = require('fs');
const os = require('os');
const path = require('path');

var platform = os.platform();
if (platform == 'darwin' || platform == 'win32') {
  var tmp = fs.realpathSync(process.env.TMPDIR);
  var foo = path.join(tmp, 'foo');
  var FOO = path.join(tmp, 'FOO');

  fs.mkdir(FOO, function(err_mk_dir_foo) {
    assert.equal(err_mk_dir_foo, null, 'no errors');
    var bar = path.join(foo, 'bar');
    var BAR = path.join(FOO, 'bar');

    fs.mkdir(BAR, function(err_mk_dir_bar) {
      assert.equal(err_mk_dir_bar, null, 'no errors');

      client.command(['watch', bar], function (error, resp) {
        assert.equal('RootResolveError: unable to resolve root ' + bar
                      + ": \"" + bar + "\" resolved to \"" + BAR
                      + "\" but we were unable to examine \""
                      + bar + "\" using strict "
                      + "case sensitive rules.  Please check each component of the path and make "
                      + "sure that that path exactly matches the correct case of the files on your "
                      + "filesystem."
                      , error.message);
        client.end();
      });
    });
  });
}
