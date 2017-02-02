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

#ifndef __PROTOBUF_UTILS_HPP__
#define __PROTOBUF_UTILS_HPP__

#include <initializer_list>
#include <set>
#include <string>

#include <sys/stat.h>

#include <mesos/mesos.hpp>

#include <mesos/maintenance/maintenance.hpp>

#include <mesos/master/master.hpp>

#include <mesos/slave/isolator.hpp>

#include <process/time.hpp>

#include <stout/duration.hpp>
#include <stout/ip.hpp>
#include <stout/none.hpp>
#include <stout/option.hpp>
#include <stout/uuid.hpp>

#include "messages/messages.hpp"

// Forward declaration (in lieu of an include).
namespace process {
struct UPID;
}

namespace mesos {
namespace internal {

namespace master {
// Forward declaration (in lieu of an include).
struct Slave;
} // namespace master {

namespace protobuf {

bool frameworkHasCapability(
    const FrameworkInfo& framework,
    FrameworkInfo::Capability::Type capability);


bool isTerminalState(const TaskState& state);


// See TaskStatus for more information about these fields. Note
// that the 'uuid' must be provided for updates that need
// acknowledgement. Currently, all slave and executor generated
// updates require acknowledgement, whereas master generated
// and scheduler driver generated updates do not.
StatusUpdate createStatusUpdate(
    const FrameworkID& frameworkId,
    const Option<SlaveID>& slaveId,
    const TaskID& taskId,
    const TaskState& state,
    const TaskStatus::Source& source,
    const Option<UUID>& uuid,
    const std::string& message = "",
    const Option<TaskStatus::Reason>& reason = None(),
    const Option<ExecutorID>& executorId = None(),
    const Option<bool>& healthy = None(),
    const Option<CheckStatusInfo>& checkStatus = None(),
    const Option<Labels>& labels = None(),
    const Option<ContainerStatus>& containerStatus = None(),
    const Option<TimeInfo>& unreachableTime = None());


StatusUpdate createStatusUpdate(
    const FrameworkID& frameworkId,
    const TaskStatus& status,
    const Option<SlaveID>& slaveId);


Task createTask(
    const TaskInfo& task,
    const TaskState& state,
    const FrameworkID& frameworkId);


Option<bool> getTaskHealth(const Task& task);


Option<CheckStatusInfo> getTaskCheckStatus(const Task& task);


Option<ContainerStatus> getTaskContainerStatus(const Task& task);


// Helper function that creates a MasterInfo from UPID.
MasterInfo createMasterInfo(const process::UPID& pid);


Label createLabel(
    const std::string& key,
    const Option<std::string>& value = None());


// Helper function that fills in a TimeInfo from the current time.
TimeInfo getCurrentTime();


// Helper function that creates a `FileInfo` from data returned by `stat()`.
FileInfo createFileInfo(const std::string& path, const struct stat& s);


ContainerID getRootContainerId(const ContainerID& containerId);

namespace slave {

struct Capabilities
{
  Capabilities() = default;

  template <typename Iterable>
  Capabilities(const Iterable& capabilities)
  {
    foreach (const SlaveInfo::Capability& capability, capabilities) {
      switch (capability.type()) {
        case SlaveInfo::Capability::UNKNOWN:
          break;
        case SlaveInfo::Capability::MULTI_ROLE:
          multiRole = true;
          break;
      }
    }
  }

  // See mesos.proto for the meaning of agent capabilities.
  bool multiRole = false;
};


mesos::slave::ContainerLimitation createContainerLimitation(
    const Resources& resources,
    const std::string& message,
    const TaskStatus::Reason& reason);


mesos::slave::ContainerState createContainerState(
    const Option<ExecutorInfo>& executorInfo,
    const ContainerID& id,
    pid_t pid,
    const std::string& directory);

} // namespace slave {

namespace maintenance {

/**
 * Helper for constructing an unavailability from a `Time` and `Duration`.
 */
Unavailability createUnavailability(
    const process::Time& start,
    const Option<Duration>& duration = None());


/**
 * Helper for constructing a list of `MachineID`.
 */
google::protobuf::RepeatedPtrField<MachineID> createMachineList(
    std::initializer_list<MachineID> ids);


/**
 * Helper for constructing a maintenance `Window`.
 * See `createUnavailability` above.
 */
mesos::maintenance::Window createWindow(
    std::initializer_list<MachineID> ids,
    const Unavailability& unavailability);


/**
 * Helper for constructing a maintenance `Schedule`.
 * See `createWindow` above.
 */
mesos::maintenance::Schedule createSchedule(
    std::initializer_list<mesos::maintenance::Window> windows);

} // namespace maintenance {

namespace master {
namespace event {

// Helper for creating a `TASK_UPDATED` event from a `Task`, its
// latest state according to the agent, and its status corresponding
// to the last status update acknowledged from the scheduler.
mesos::master::Event createTaskUpdated(
    const Task& task,
    const TaskState& state,
    const TaskStatus& status);


// Helper for creating a `TASK_ADDED` event from a `Task`.
mesos::master::Event createTaskAdded(const Task& task);


// Helper for creating an `Agent` response.
mesos::master::Response::GetAgents::Agent createAgentResponse(
    const mesos::internal::master::Slave& slave);


// Helper for creating an `AGENT_ADDED` event from a `Slave`.
mesos::master::Event createAgentAdded(
    const mesos::internal::master::Slave& slave);


// Helper for creating an `AGENT_REMOVED` event from a `SlaveID`.
mesos::master::Event createAgentRemoved(const SlaveID& slaveId);

} // namespace event {
} // namespace master {

namespace framework {

struct Capabilities
{
  Capabilities() = default;

  template <typename Iterable>
  Capabilities(const Iterable& capabilities)
  {
    foreach (const FrameworkInfo::Capability& capability, capabilities) {
      switch (capability.type()) {
        case FrameworkInfo::Capability::UNKNOWN:
          break;
        case FrameworkInfo::Capability::REVOCABLE_RESOURCES:
          revocableResources = true;
          break;
        case FrameworkInfo::Capability::TASK_KILLING_STATE:
          taskKillingState = true;
          break;
        case FrameworkInfo::Capability::GPU_RESOURCES:
          gpuResources = true;
          break;
        case FrameworkInfo::Capability::SHARED_RESOURCES:
          sharedResources = true;
          break;
        case FrameworkInfo::Capability::PARTITION_AWARE:
          partitionAware = true;
          break;
        case FrameworkInfo::Capability::MULTI_ROLE:
          multiRole = true;
          break;
      }
    }
  }

  // See mesos.proto for the meaning of these capabilities.
  bool revocableResources = false;
  bool taskKillingState = false;
  bool gpuResources = false;
  bool sharedResources = false;
  bool partitionAware = false;
  bool multiRole = false;
};


// Helper to get roles from FrameworkInfo based on the
// presence of the MULTI_ROLE capability.
std::set<std::string> getRoles(const FrameworkInfo& frameworkInfo);

} // namespace framework {

} // namespace protobuf {
} // namespace internal {
} // namespace mesos {

#endif // __PROTOBUF_UTILS_HPP__
