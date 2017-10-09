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

#pragma once



// ***************
//  STD Libraries
// ***************

#include <map>



// ***********
//  API Begin
// ***********

namespace swarm {
    namespace synchro {

        class Lock;
        class SharedLock;
        class UpgradeLock;

        //! Interface for a mutex lockable object
        /*!
         * Interface providing mutex lock and unlock methods. Provided methods are
         * protected and meant to be used by subclasses rather than external sources.
         *
         * Contains its own internal boost::mutex.
         */
        class Lockable {
        public:
            Lockable();
            virtual ~Lockable();
            struct InternMutex;
        protected:

            //! Locks the internal mutex
            /*!
             * Locks this object's internal mutex. Essentially a wrapped call to
             * a boost::mutex's lock() method. Follows all rules for Boost's
             * mutexes.
             */
            void lockMutex();

            //! Unlocks the internal mutex
            /*!
             * Unlocks this object's internal mutex. Essentially a wrapped call to
             * a boost::mutex's unlock() method. Follows all rules for Boost's
             * mutexes.
             */
            void unlockMutex();

        private:
            friend class Lock;
            InternMutex* mMutex;
        };

        //! Interface for a shared mutex lockable object
        /*!
         * Interface providing shared mutex lock and unlock methods (many reads, single
         * write). Provided methods are protected and meant to be used by subclasses
         * rather than external sources.
         *
         * Contains its own internal boost::shared_mutex.
         */
        class SharedLockable {
        public:
            SharedLockable();
            virtual ~SharedLockable();
            struct InternSharedMutex;
        protected:

            //! Exclusively locks the internal mutex
            /*!
             * Exclusively locks this object's internal mutex. Essentially a
             * wrapped call to a boost::shared_mutex's lock() method. Follows
             * all rules for Boost's mutexes.
             *
             * Use this method for writing.
             */
            void lockMutex();

            //! Exclusively unlocks the internal mutex
            /*!
             * Exclusively unlocks this object's internal mutex. Essentially a
             * wrapped call to a boost::shared_mutex's unlock() method. Follows
             * all rules for Boost's mutexes.
             *
             * Use this method for writing.
             */
            void unlockMutex();

            //! Shared locks the internal mutex
            /*!
             * Shared locks this object's internal mutex. Essentially a
             * wrapped call to a boost::shared_mutex's lock_shared() method.
             * Follows all rules for Boost's mutexes.
             *
             * Use this method for reading. Many threads can obtain a shared
             * lock at the same time.
             */
            void lockMutexShared();

            //! Shared unlocks the internal mutex
            /*!
             * Shared unlocks this object's internal mutex. Essentially a
             * wrapped call to a boost::shared_mutex's unlock_shared() method.
             * Follows all rules for Boost's mutexes.
             *
             * Use this method for reading. Many threads can obtain a shared
             * lock at the same time.
             */
            void unlockMutexShared();

        private:
            friend class Lock;
            friend class SharedLock;
            friend class UpgradeLock;
            InternSharedMutex* mMutex;
        };

        //! RAII Exclusive Lock
        /*!
         * An RAII lock to obtain exclusive access to a mutex. Can be created
         * from either a Lockable or SharedLockable object.
         *
         * Locks when constructed, and unlocks when destructed. Only a single
         * Lock can exist for a mutex at once.
         */
        class Lock {
        public:
            explicit Lock(const Lockable& pLockable);
            explicit Lock(const SharedLockable& pLockable);
            ~Lock();
            struct InternLock;
        private:
            InternLock* mLock;
        };

        //! RAII Shared Lock
        /*!
         * An RAII lock to obtain shared access to a mutex. Can be created
         * from a SharedLockable object. Should be used for read access
         * only.
         *
         * Locks when constructed, and unlocks when destructed. Many
         * SharedLocks can exist for a single mutex at the same time.
         */
        class SharedLock {
        public:
            explicit SharedLock(const SharedLockable& pLockable);
            ~SharedLock();
            struct InternSharedLock;
        private:
            InternSharedLock* mLock;
        };

        //! RAII Upgrade Lock
        /*!
         * An RAII lock to obtain shared access to a mutex which can be
         * upgraded to exclusive access a later time. Can be created
         * from a SharedLockable object. Should be used for read access
         * only while not upgraded.
         *
         * Locks when constructed, and unlocks when destructed. Only a single
         * UpgradeLock can exist for a mutex at a time.
         *
         * Once an UpgradeLock has been obtained, it can be upgraded
         * to exclusive access with upgrade().
         */
        class UpgradeLock {
        public:
            explicit UpgradeLock(const SharedLockable& pLockable);
            ~UpgradeLock();
            struct InternUpgradeLock;

            //! Upgrades to an exclusive lock
            /*!
             * Upgrades this shared lock to an exclusive lock to be used
             * for write access. If this method is called on a shared lock
             * that has already been upgrades, it will throw an exception.
             */
            void upgrade();

        private:
            InternUpgradeLock* mLock;
            bool upgraded = false;
        };

    }
}