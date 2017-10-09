/*
 * Copyright 2017 James De Broeck
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

#include <SwarmSynchro.h>
#include "gtest/gtest.h"
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <unordered_set>
#include <list>

#define BULK_COUNT 50


namespace {

    class AsyncIncrementable {
    public:
        virtual void increment_with_guard() = 0;
        virtual void increment_with_lock() = 0;
    };

    struct async_increment_callable_with_guard {
        AsyncIncrementable& mRef;
        explicit async_increment_callable_with_guard(AsyncIncrementable& pRef) : mRef(pRef) {}
        void operator()() { mRef.increment_with_guard(); }
    };

    struct async_increment_callable_with_lock {
        AsyncIncrementable& mRef;
        explicit async_increment_callable_with_lock(AsyncIncrementable& pRef) : mRef(pRef) {}
        void operator()() { mRef.increment_with_lock(); }
    };

    bool joinThread(boost::thread& call, int milliseconds) {
        bool joinable = call.joinable();
        bool joined = call.try_join_for(boost::chrono::milliseconds(milliseconds));
        return joined || !joinable;
    }

    class LockableTest : public ::testing::Test, public swarm::synchro::Lockable, public AsyncIncrementable {
    protected:
        int mNum = 0;
    public:
        void increment_with_lock() override {
            lockMutex();
            mNum++;
            unlockMutex();
        }
        void increment_with_guard() override {
            swarm::synchro::Lock lock(*this);
            mNum++;
        }
    };

    TEST_F(LockableTest, ExclusiveLockTest) {
        lockMutex();
        boost::thread async_call(async_increment_callable_with_lock(*this));
        EXPECT_EQ(0, mNum);
        unlockMutex();
        EXPECT_TRUE(joinThread(async_call, 500));
        EXPECT_EQ(1, mNum);
        increment_with_lock();
        EXPECT_EQ(2, mNum);
    }

    TEST_F(LockableTest, ExclusiveGuardTest) {
        lockMutex();
        boost::thread async_call(async_increment_callable_with_guard(*this));
        EXPECT_EQ(0, mNum);
        unlockMutex();
        EXPECT_TRUE(joinThread(async_call, 500));
        EXPECT_EQ(1, mNum);
        increment_with_guard();
        EXPECT_EQ(2, mNum);
    }

    TEST_F(LockableTest, BulkLockWriteTest) {
        std::list<boost::thread*> allThreads;
        for(int i = 0; i < BULK_COUNT; i++) {
            allThreads.push_back(new boost::thread(async_increment_callable_with_lock(*this)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        EXPECT_EQ(BULK_COUNT, mNum);
        for(boost::thread* call : allThreads)
            delete call;
    }

    TEST_F(LockableTest, BulkGuardWriteTest) {
        std::list<boost::thread*> allThreads;
        for(int i = 0; i < BULK_COUNT; i++) {
            allThreads.push_back(new boost::thread(async_increment_callable_with_guard(*this)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        EXPECT_EQ(BULK_COUNT, mNum);
        for(boost::thread* call : allThreads)
            delete call;
    }

    class AsyncReadable {
    public:
        virtual int get_value_with_lock() = 0;
        virtual int get_value_with_guard() = 0;
        virtual int get_value_unsafe() = 0;
    };

    struct async_read_callable_with_guard {
        AsyncReadable& mRef;
        int& mNum;
        explicit async_read_callable_with_guard(AsyncReadable& pRef, int& pNum) : mRef(pRef), mNum(pNum) {}
        void operator()() { mNum = mRef.get_value_with_guard(); }
    };

    struct async_read_callable_with_lock {
        AsyncReadable& mRef;
        int& mNum;
        explicit async_read_callable_with_lock(AsyncReadable& pRef, int& pNum) : mRef(pRef), mNum(pNum) {}
        void operator()() { mNum = mRef.get_value_with_lock(); }
    };

    class SharedLockableTest :
            public ::testing::Test,
            public swarm::synchro::SharedLockable,
            public AsyncIncrementable,
            public AsyncReadable {
    protected:
        int mNum = 0;
    public:
        void increment_with_lock() override {
            lockMutex();
            mNum++;
            unlockMutex();
        }
        void increment_with_guard() override {
            swarm::synchro::Lock lock(*this);
            mNum++;
        }
        int get_value_with_lock() override {
            lockMutexShared();
            int result = mNum;
            unlockMutexShared();
            return result;
        }
        int get_value_with_guard() override {
            swarm::synchro::SharedLock lock(*this);
            return mNum;
        }
        int get_value_unsafe() override {
            return mNum;
        }
    };

    TEST_F(SharedLockableTest, ExclusiveLockTest) {
        lockMutexShared();
        boost::thread async_call(async_increment_callable_with_lock(*this));
        EXPECT_EQ(0, get_value_unsafe());
        unlockMutexShared();
        EXPECT_TRUE(joinThread(async_call, 500));
        EXPECT_EQ(1, get_value_with_lock());
        increment_with_lock();
        EXPECT_EQ(2, get_value_with_lock());
    }

    TEST_F(SharedLockableTest, ExclusiveGuardTest) {
        lockMutexShared();
        boost::thread async_call(async_increment_callable_with_guard(*this));
        EXPECT_EQ(0, get_value_unsafe());
        unlockMutexShared();
        EXPECT_TRUE(joinThread(async_call, 500));
        EXPECT_EQ(1, get_value_with_guard());
        increment_with_lock();
        EXPECT_EQ(2, get_value_with_guard());
    }

    TEST_F(SharedLockableTest, BulkLockWriteTest) {
        std::list<boost::thread*> allThreads;
        for(int i = 0; i < BULK_COUNT; i++) {
            allThreads.push_back(new boost::thread(async_increment_callable_with_lock(*this)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        EXPECT_EQ(BULK_COUNT, get_value_with_lock());
        for(boost::thread* call : allThreads)
            delete call;
    }

    TEST_F(SharedLockableTest, BulkGuardWriteTest) {
        std::list<boost::thread*> allThreads;
        for(int i = 0; i < BULK_COUNT; i++) {
            allThreads.push_back(new boost::thread(async_increment_callable_with_guard(*this)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        EXPECT_EQ(BULK_COUNT, get_value_with_guard());
        for(boost::thread* call : allThreads)
            delete call;
    }

    TEST_F(SharedLockableTest, BulkLockReadTest) {
        std::list<boost::thread*> allThreads;
        int allNums[BULK_COUNT];
        increment_with_lock();
        int expectedVal = get_value_with_lock();
        lockMutexShared();
        for (int &num : allNums) {
            allThreads.push_back(new boost::thread(async_read_callable_with_lock(*this, num)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        unlockMutexShared();
        int failedReads = 0;
        for (int &num : allNums) {
            if(num != expectedVal)
                failedReads++;
        }
        EXPECT_EQ(0, failedReads);
        for(boost::thread* call : allThreads)
            delete call;
    }

    TEST_F(SharedLockableTest, BulkGuardReadTest) {
        std::list<boost::thread*> allThreads;
        int allNums[BULK_COUNT];
        increment_with_guard();
        int expectedVal = get_value_with_guard();
        swarm::synchro::SharedLock lock(*this);
        for (int &num : allNums) {
            allThreads.push_back(new boost::thread(async_read_callable_with_guard(*this, num)));
        }
        int threadsNotJoined = 0;
        for(boost::thread* call : allThreads) {
            if(!joinThread(*call, 150))
                threadsNotJoined++;
        }
        EXPECT_EQ(0, threadsNotJoined);
        int failedReads = 0;
        for (int &num : allNums) {
            if(num != expectedVal)
                failedReads++;
        }
        EXPECT_EQ(0, failedReads);
        for(boost::thread* call : allThreads)
            delete call;
    }

    TEST_F(SharedLockableTest, UpgradeLockTest) {
        int expectedVal = mNum;
        int val = -1;
        auto lock = new swarm::synchro::UpgradeLock(*this);
        boost::thread async_call_1(async_read_callable_with_guard(*this, val));
        EXPECT_TRUE(joinThread(async_call_1, 500));
        EXPECT_EQ(expectedVal, val);
        lock->upgrade();
        val = -1;
        boost::thread async_call_2(async_read_callable_with_guard(*this, val));
        mNum = 42; expectedVal = mNum;
        EXPECT_FALSE(joinThread(async_call_2, 500));
        delete lock;
        EXPECT_TRUE(joinThread(async_call_2, 500));
        EXPECT_EQ(expectedVal, val);
    }
}