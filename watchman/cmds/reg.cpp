/*
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

#include <folly/ExceptionString.h>
#include <folly/ScopeGuard.h>
#include <folly/Synchronized.h>
#include "watchman/CommandRegistry.h"
#include "watchman/Logging.h"
#include "watchman/Poison.h"
#include "watchman/WatchmanConfig.h"
#include "watchman/thirdparty/jansson/jansson.h"
#include "watchman/watchman_client.h"
#include "watchman/watchman_cmd.h"
#include "watchman/watchman_pdu.h"
#include "watchman/watchman_stream.h"

using namespace watchman;

command_handler_def* lookup(const json_ref& args, int mode) {
  const char* cmd_name;

  if (!json_array_size(args)) {
    throw CommandValidationError(
        "invalid command (expected an array with some elements!)");
  }

  const auto jstr = json_array_get(args, 0);
  cmd_name = json_string_value(jstr);
  if (!cmd_name) {
    throw CommandValidationError(
        "invalid command: expected element 0 to be the command name");
  }

  return lookup_command(json_to_w_string(jstr), mode);
}

void preprocess_command(
    json_ref& args,
    enum w_pdu_type output_pdu,
    uint32_t output_capabilities) {
  command_handler_def* def;

  try {
    def = lookup(args, 0);

    if (!def) {
      // Nothing known about it, pass the command on anyway for forwards
      // compatibility
      return;
    }

    if (def->cli_validate) {
      def->cli_validate(args);
    }
  } catch (const std::exception& exc) {
    w_jbuffer_t jr;

    auto err = json_object(
        {{"error", typed_string_to_json(exc.what(), W_STRING_MIXED)},
         {"version", typed_string_to_json(PACKAGE_VERSION, W_STRING_UNICODE)},
         {"cli_validated", json_true()}});

    jr.pduEncodeToStream(output_pdu, output_capabilities, err, w_stm_stdout());
    exit(1);
  }
}

bool dispatch_command(
    struct watchman_client* client,
    const json_ref& args,
    int mode) {
  command_handler_def* def;
  char sample_name[128];

  // Stash a reference to the current command to make it easier to log
  // the command context in some of the error paths
  client->current_command = args;
  SCOPE_EXIT {
    client->current_command = nullptr;
  };

  try {
    def = lookup(args, mode);
    if (!def) {
      send_error_response(client, "Unknown command");
      return false;
    }

    if (!poisoned_reason.rlock()->empty() &&
        (def->flags & CMD_POISON_IMMUNE) == 0) {
      send_error_response(client, "%s", poisoned_reason.rlock()->c_str());
      return false;
    }

    if (!client->client_is_owner && (def->flags & CMD_ALLOW_ANY_USER) == 0) {
      send_error_response(
          client, "you must be the process owner to execute '%s'", def->name);
      return false;
    }

    // Scope for the perf sample
    {
      logf(DBG, "dispatch_command: {}\n", def->name);
      snprintf(
          sample_name, sizeof(sample_name), "dispatch_command:%s", def->name);
      PerfSample sample(sample_name);
      client->perf_sample = &sample;
      SCOPE_EXIT {
        client->perf_sample = nullptr;
      };

      sample.set_wall_time_thresh(
          cfg_get_double("slow_command_log_threshold_seconds", 1.0));

      def->func(client, args);

      if (sample.finish()) {
        sample.add_meta("args", json_ref(args));
        sample.add_meta(
            "client",
            json_object(
                {{"pid", json_integer(client->stm->getPeerProcessID())}}));
        sample.log();
      }
      logf(DBG, "dispatch_command: {} (completed)\n", def->name);
    }

    return true;
  } catch (const std::exception& e) {
    auto what = folly::exceptionStr(e);
    send_error_response(client, "%s", what.c_str());
    return false;
  }
}

/* vim:ts=2:sw=2:et:
 */
