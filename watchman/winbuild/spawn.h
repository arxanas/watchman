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

#pragma once

// Spawn attributes

typedef struct _posix_spawnattr {
  short flags;
  char* working_dir;
} posix_spawnattr_t;

#define POSIX_SPAWN_SETPGROUP 2

int posix_spawnattr_init(posix_spawnattr_t* attrp);
int posix_spawnattr_setflags(posix_spawnattr_t* attrp, short flags);
int posix_spawnattr_getflags(posix_spawnattr_t* attrp, short* flags);
int posix_spawnattr_destroy(posix_spawnattr_t* attrp);

int posix_spawnattr_setcwd_np(posix_spawnattr_t* attrp, const char* path);

// File actions
struct _posix_spawn_file_action {
  enum { open_file, dup_fd, dup_handle } action;
  int target_fd;
  union {
    intptr_t dup_local_handle;
    int source_fd;
    struct {
      char* name;
      int flags;
      int mode;
    } open_info;
  } u;
};

typedef struct _posix_spawn_file_actions {
  struct _posix_spawn_file_action* acts;
  int nacts;
} posix_spawn_file_actions_t;

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* actions);
int posix_spawn_file_actions_adddup2(
    posix_spawn_file_actions_t* actions,
    int fd,
    int target_fd);
int posix_spawn_file_actions_adddup2_handle_np(
    posix_spawn_file_actions_t* actions,
    intptr_t handle,
    int target_fd);
int posix_spawn_file_actions_addopen(
    posix_spawn_file_actions_t* actions,
    int target_fd,
    const char* name,
    int flags,
    int mode);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* actions);

// And spawning itself

int posix_spawn(
    pid_t* pid,
    const char* path,
    const posix_spawn_file_actions_t* file_actions,
    const posix_spawnattr_t* attrp,
    char* const argv[],
    char* const envp[]);
int posix_spawnp(
    pid_t* pid,
    const char* file,
    const posix_spawn_file_actions_t* file_actions,
    const posix_spawnattr_t* attrp,
    char* const argv[],
    char* const envp[]);

#define WNOHANG 1
pid_t waitpid(pid_t pid, int* status, int options);
