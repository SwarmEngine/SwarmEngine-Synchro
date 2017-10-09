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

#include "../SwarmSynchro.h"

#include <unordered_map>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_types.hpp>

namespace swarm {
    namespace synchro {



        // **********
        //  Lockable
        // **********

        struct Lockable::InternMutex {
        public:
            boost::mutex value;
        };

        void Lockable::lockMutex() {
            mMutex->value.lock();
        }

        void Lockable::unlockMutex() {
            mMutex->value.unlock();
        }

        Lockable::Lockable() {
            mMutex = new Lockable::InternMutex();
        }

        Lockable::~Lockable() {
            delete mMutex;
        }



        // ****************
        //  SharedLockable
        // ****************

        struct SharedLockable::InternSharedMutex {
        public:
            boost::upgrade_mutex value;
        };

        void SharedLockable::lockMutex() {
            mMutex->value.lock();
        }

        void SharedLockable::unlockMutex() {
            mMutex->value.unlock();
        }

        void SharedLockable::lockMutexShared() {
            mMutex->value.lock_shared();
        }

        void SharedLockable::unlockMutexShared() {
            mMutex->value.unlock_shared();
        }

        SharedLockable::SharedLockable() {
            mMutex = new SharedLockable::InternSharedMutex();
        }

        SharedLockable::~SharedLockable() {
            delete mMutex;
        }



        // ******
        //  Lock
        // ******

        struct Lock::InternLock {
            virtual ~InternLock() = default;
        };

        struct InternLock_Normal : public Lock::InternLock {
        public:
            boost::unique_lock<boost::mutex>* value;
            explicit InternLock_Normal(boost::mutex& pMutex) {
                    value = new boost::unique_lock<boost::mutex>(pMutex);
            }
            ~InternLock_Normal() override {
                delete value;
            }
        };

        struct InternLock_Upgrade : public Lock::InternLock {
        public:
            boost::unique_lock<boost::upgrade_mutex>* value;
            explicit InternLock_Upgrade(boost::upgrade_mutex& pMutex) {
                value = new boost::unique_lock<boost::upgrade_mutex>(pMutex);
            }
            ~InternLock_Upgrade() override {
                delete value;
            }
        };

        Lock::Lock(const Lockable& pLockable) {
            mLock = new InternLock_Normal(pLockable.mMutex->value);
        }

        Lock::Lock(const SharedLockable& pLockable) {
            mLock = new InternLock_Upgrade(pLockable.mMutex->value);
        }

        Lock::~Lock() {
            delete mLock;
        }



        // ************
        //  SharedLock
        // ************

        struct SharedLock::InternSharedLock {
            virtual ~InternSharedLock() = default;
        };

        struct InternSharedLock_Normal : public SharedLock::InternSharedLock {
        public:
            boost::shared_lock<boost::upgrade_mutex>* value;
            explicit InternSharedLock_Normal(boost::upgrade_mutex& pMutex) {
                value = new boost::shared_lock<boost::upgrade_mutex>(pMutex);
            }
            ~InternSharedLock_Normal() override {
                delete value;
            }
        };

        struct InternSharedLock_Unique : public SharedLock::InternSharedLock {
        public:
            boost::upgrade_to_unique_lock<boost::upgrade_mutex>* value;
            explicit InternSharedLock_Unique(boost::upgrade_lock<boost::upgrade_mutex>& pMutex) {
                value = new boost::upgrade_to_unique_lock<boost::upgrade_mutex>(pMutex);
            }
            ~InternSharedLock_Unique() override {
                delete value;
            }
        };

        SharedLock::SharedLock(const SharedLockable& pLockable) {
            mLock = new InternSharedLock_Normal(pLockable.mMutex->value);
        }

        SharedLock::~SharedLock() {
            delete mLock;
        }



        // *************
        //  UpgradeLock
        // *************

        struct UpgradeLock::InternUpgradeLock {
            virtual ~InternUpgradeLock() = default;
        };

        struct InternUpgradeLock_Normal : public UpgradeLock::InternUpgradeLock {
        public:
            boost::upgrade_lock<boost::upgrade_mutex>* value;
            explicit InternUpgradeLock_Normal(boost::upgrade_mutex& pMutex) {
                value = new boost::upgrade_lock<boost::upgrade_mutex>(pMutex);
            }
            ~InternUpgradeLock_Normal() override {
                delete value;
            }
        };

        struct InternUpgradeLock_Unique : public UpgradeLock::InternUpgradeLock {
        public:
            boost::upgrade_to_unique_lock<boost::upgrade_mutex>* value;
            explicit InternUpgradeLock_Unique(boost::upgrade_lock<boost::upgrade_mutex>& pMutex) {
                value = new boost::upgrade_to_unique_lock<boost::upgrade_mutex>(pMutex);
            }
            ~InternUpgradeLock_Unique() override {
                delete value;
            }
        };

        UpgradeLock::UpgradeLock(const SharedLockable& pLockable) {
            mLock = new InternUpgradeLock_Normal(pLockable.mMutex->value);
        }

        UpgradeLock::~UpgradeLock() {
            delete mLock;
        }

        void UpgradeLock::upgrade() {
            if(upgraded)
                throw std::logic_error("Attempted to upgrade to unique lock on upgrade_mutex that has already been upgraded.");
            auto oldLock = (InternUpgradeLock_Normal*)mLock;
            mLock = new InternUpgradeLock_Unique(*oldLock->value);
            upgraded = true;
            delete oldLock;
        }

    }
}