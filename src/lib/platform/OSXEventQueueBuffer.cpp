/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/OSXEventQueueBuffer.h"

#include "base/Event.h"
#include "base/IEventQueue.h"

#include <algorithm>
#include <chrono>

namespace inputleap {

OSXEventQueueBuffer::OSXEventQueueBuffer(IEventQueue* events) :
    m_event(nullptr),
    m_eventQueue(events),
    m_carbonEventQueue(nullptr)
{
    // do nothing
}

OSXEventQueueBuffer::~OSXEventQueueBuffer()
{
    // release the last event
    if (m_event != nullptr) {
        ReleaseEvent(m_event);
    }
}

void
OSXEventQueueBuffer::init()
{
    m_carbonEventQueue = GetCurrentEventQueue();
}

void
OSXEventQueueBuffer::waitForEvent(double timeout)
{
    // PostEventToQueue() no longer reliably interrupts ReceiveNextEvent() when
    // the producer is a different thread on current macOS.  Use an explicit
    // condition variable for InputLeap events, retaining a short timeout so
    // native Carbon events are still observed.
    std::unique_lock<std::mutex> lock(m_wakeMutex);
    if (!isEmpty()) {
        return;
    }

    constexpr auto nativeEventPollInterval = std::chrono::milliseconds(50);
    if (timeout < 0.0) {
        m_wakeCondition.wait_for(lock, nativeEventPollInterval);
    }
    else {
        const auto requested = std::chrono::duration<double>(timeout);
        m_wakeCondition.wait_for(lock, std::min(requested,
                                                std::chrono::duration<double>(nativeEventPollInterval)));
    }
}

IEventQueueBuffer::Type OSXEventQueueBuffer::getEvent(Event& event, std::uint32_t& dataID)
{
    // release the previous event
    if (m_event != nullptr) {
        ReleaseEvent(m_event);
        m_event = nullptr;
    }

    // get the next event
    OSStatus error = ReceiveNextEvent(0, nullptr, 0.0, true, &m_event);

    // handle the event
    if (error == eventLoopQuitErr) {
        event = Event(EventType::QUIT);
        return kSystem;
    }
    else if (error != noErr) {
        return kNone;
    }
    else {
        std::uint32_t eventClass = GetEventClass(m_event);
        switch (eventClass) {
        case 'Syne':
            dataID = GetEventKind(m_event);
            return kUser;

        default:
            event = Event(EventType::SYSTEM, m_eventQueue->getSystemTarget(),
                          create_event_data<EventRef*>(&m_event));
            return kSystem;
        }
    }
}

bool OSXEventQueueBuffer::addEvent(std::uint32_t dataID)
{
    EventRef event;
    OSStatus error = CreateEvent(
                            kCFAllocatorDefault,
                            'Syne',
                            dataID,
                            0,
                            kEventAttributeNone,
                            &event);

    if (error == noErr) {
        assert(m_carbonEventQueue != nullptr);

        {
            std::lock_guard<std::mutex> lock(m_wakeMutex);
            error = PostEventToQueue(
                m_carbonEventQueue,
                event,
                kEventPriorityStandard);
        }

        ReleaseEvent(event);
        m_wakeCondition.notify_one();
    }

    return (error == noErr);
}

bool
OSXEventQueueBuffer::isEmpty() const
{
    EventRef event;
    OSStatus status = ReceiveNextEvent(0, nullptr, 0.0, false, &event);
    return (status == eventLoopTimedOutErr);
}

} // namespace inputleap
