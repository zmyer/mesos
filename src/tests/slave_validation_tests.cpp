// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <mesos/mesos.hpp>

#include <mesos/agent/agent.hpp>

#include <stout/error.hpp>
#include <stout/gtest.hpp>
#include <stout/option.hpp>
#include <stout/uuid.hpp>

#include "slave/slave.hpp"
#include "slave/validation.hpp"

#include "tests/mesos.hpp"

namespace validation = mesos::internal::slave::validation;

using mesos::internal::slave::Slave;

namespace mesos {
namespace internal {
namespace tests {

TEST(AgentValidationTest, ContainerID)
{
  ContainerID containerId;
  Option<Error> error;

  // No empty IDs.
  containerId.set_value("");
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  // No slashes.
  containerId.set_value("/");
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  containerId.set_value("\\");
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  // No spaces.
  containerId.set_value(" ");
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  // No periods.
  containerId.set_value(".");
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  // Valid.
  containerId.set_value("redis");
  error = validation::container::validateContainerId(containerId);
  EXPECT_NONE(error);

  // Valid with invalid parent (empty `ContainerID.value`).
  containerId.set_value("backup");
  containerId.mutable_parent();
  error = validation::container::validateContainerId(containerId);
  EXPECT_SOME(error);

  // Valid with valid parent.
  containerId.set_value("backup");
  containerId.mutable_parent()->set_value("redis");
  error = validation::container::validateContainerId(containerId);
  EXPECT_NONE(error);
}


TEST(AgentCallValidationTest, LaunchNestedContainer)
{
  // Missing `launch_nested_container`.
  agent::Call call;
  call.set_type(agent::Call::LAUNCH_NESTED_CONTAINER);

  Option<Error> error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // `container_id` is not valid.
  ContainerID badContainerId;
  badContainerId.set_value("no spaces allowed");

  agent::Call::LaunchNestedContainer* launch =
    call.mutable_launch_nested_container();

  launch->mutable_container_id()->CopyFrom(badContainerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Valid `container_id` but missing `container_id.parent`.
  ContainerID containerId;
  containerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Valid `container_id.parent` but invalid `command.environment`. Currently,
  // `Environment.Variable.Value` must be set, but this constraint will be
  // removed in a future version.
  ContainerID parentContainerId;
  parentContainerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->mutable_parent()->CopyFrom(parentContainerId);
  launch->mutable_command()->CopyFrom(createCommandInfo("exit 0"));

  Environment::Variable* variable = launch
    ->mutable_command()
    ->mutable_environment()
    ->mutable_variables()
    ->Add();
  variable->set_name("ENV_VAR_KEY");

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);
  EXPECT_EQ(
      "'launch_nested_container.command' is invalid: Environment variable "
      "'ENV_VAR_KEY' must have a value set",
      error->message);

  // Test the valid case.
  variable->set_value("env_var_value");
  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);

  // Any number of parents is valid.
  ContainerID grandparentContainerId;
  grandparentContainerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->mutable_parent()->mutable_parent()
    ->CopyFrom(grandparentContainerId);

  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);
}


TEST(AgentCallValidationTest, WaitNestedContainer)
{
  // Missing `wait_nested_container`.
  agent::Call call;
  call.set_type(agent::Call::WAIT_NESTED_CONTAINER);

  Option<Error> error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Expecting a `container_id.parent`.
  ContainerID containerId;
  containerId.set_value(UUID::random().toString());

  agent::Call::WaitNestedContainer* wait =
    call.mutable_wait_nested_container();

  wait->mutable_container_id()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Test the valid case.
  ContainerID parentContainerId;
  parentContainerId.set_value(UUID::random().toString());

  wait->mutable_container_id()->mutable_parent()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);
}


TEST(AgentCallValidationTest, KillNestedContainer)
{
  // Missing `kill_nested_container`.
  agent::Call call;
  call.set_type(agent::Call::KILL_NESTED_CONTAINER);

  Option<Error> error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Expecting a `container_id.parent`.
  ContainerID containerId;
  containerId.set_value(UUID::random().toString());

  agent::Call::KillNestedContainer* kill =
    call.mutable_kill_nested_container();

  kill->mutable_container_id()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Test the valid case.
  ContainerID parentContainerId;
  parentContainerId.set_value(UUID::random().toString());

  kill->mutable_container_id()->mutable_parent()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);
}


TEST(AgentCallValidationTest, LaunchNestedContainerSession)
{
  // Missing `launch_nested_container_session`.
  agent::Call call;
  call.set_type(agent::Call::LAUNCH_NESTED_CONTAINER_SESSION);

  Option<Error> error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // `container_id` is not valid.
  ContainerID badContainerId;
  badContainerId.set_value("no spaces allowed");

  agent::Call::LaunchNestedContainerSession* launch =
    call.mutable_launch_nested_container_session();

  launch->mutable_container_id()->CopyFrom(badContainerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Valid `container_id` but missing `container_id.parent`.
  ContainerID containerId;
  containerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->CopyFrom(containerId);

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);

  // Valid `container_id.parent` but invalid `command.environment`. Currently,
  // `Environment.Variable.Value` must be set, but this constraint will be
  // removed in a future version.
  ContainerID parentContainerId;
  parentContainerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->mutable_parent()->CopyFrom(parentContainerId);
  launch->mutable_command()->CopyFrom(createCommandInfo("exit 0"));

  Environment::Variable* variable = launch
    ->mutable_command()
    ->mutable_environment()
    ->mutable_variables()
    ->Add();
  variable->set_name("ENV_VAR_KEY");

  error = validation::agent::call::validate(call);
  EXPECT_SOME(error);
  EXPECT_EQ(
      "'launch_nested_container_session.command' is invalid: Environment "
      "variable 'ENV_VAR_KEY' must have a value set",
      error->message);

  // Test the valid case.
  variable->set_value("env_var_value");
  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);

  // Any number of parents is valid.
  ContainerID grandparentContainerId;
  grandparentContainerId.set_value(UUID::random().toString());

  launch->mutable_container_id()->mutable_parent()->mutable_parent()->CopyFrom(
      grandparentContainerId);

  error = validation::agent::call::validate(call);
  EXPECT_NONE(error);
}

} // namespace tests {
} // namespace internal {
} // namespace mesos {
