/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Assertion.hpp>
#include <WCDB/CheckpointQueue.hpp>
#include <WCDB/CoreConst.h>

namespace WCDB {

CheckpointEvent::~CheckpointEvent()
{
}

CheckpointQueue::CheckpointQueue(const String& name, CheckpointEvent* event)
: AsyncQueue(name, event)
, m_checkpointPassive(StatementPragma().pragma(Pragma::walCheckpoint()).with("PASSIVE"))
, m_checkpointTruncate(StatementPragma().pragma(Pragma::walCheckpoint()).with("TRUNCATE"))
{
}

void CheckpointQueue::loop()
{
    m_timedQueue.loop(std::bind(
    &CheckpointQueue::onTimed, this, std::placeholders::_1, std::placeholders::_2));
}

bool CheckpointQueue::onTimed(const String& path, const int& frames)
{
    bool result;
    if (frames >= CheckpointQueueFramesThresholdForTruncating) {
        result = static_cast<CheckpointEvent*>(m_event)->databaseShouldCheckpoint(
        path, m_checkpointTruncate);
    } else {
        result = static_cast<CheckpointEvent*>(m_event)->databaseShouldCheckpoint(
        path, m_checkpointPassive);
    }
    if (!result) {
        // delay retry if failed
        m_timedQueue.reQueue(path, CheckpointQueueDelayForRetryingAfterFailure, frames);
    }
    return result;
}

void CheckpointQueue::put(const String& path, double delay, int frames)
{
    m_timedQueue.reQueue(path, delay, frames);
    lazyRun();
}

void CheckpointQueue::remove(const String& path)
{
    m_timedQueue.remove(path);
}

} // namespace WCDB
