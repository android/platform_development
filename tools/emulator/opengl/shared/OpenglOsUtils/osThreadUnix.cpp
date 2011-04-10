/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "osThread.h"

namespace osUtils {

Thread::Thread() :
    m_thread((pthread_t)NULL),
    m_isRunning(false)
{
}

Thread::~Thread()
{
}

bool
Thread::start()
{
    m_isRunning = true;
    int ret = pthread_create(&m_thread, NULL, Thread::thread_main, this);
    if(ret) {
        m_isRunning = false;
    }
    return m_isRunning;
}

bool
Thread::wait(int *exitStatus)
{
    if (!m_isRunning) {
        return false;
    }

    void *retval;
    if (pthread_join(m_thread,&retval)) {
        return false;
    }
    m_isRunning = false;
    long long int ret=(long long int)retval;
    if (exitStatus) {
        *exitStatus = (int)ret;
    }
    return true;
}

bool
Thread::trywait(int *exitStatus)
{
    // XXX: not implemented for linux
    return false;
}

void *
Thread::thread_main(void *p_arg)
{
    Thread *self = (Thread *)p_arg;
    void *ret = (void *)self->Main();
    self->m_isRunning = false;
    return ret;
}

} // of namespace osUtils

